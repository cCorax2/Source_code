#include "stdafx.h"
#include <sstream>

#include "desc.h"
#include "desc_manager.h"
#include "char.h"
#include "buffer_manager.h"
#include "config.h"
#include "profiler.h"
#include "p2p.h"
#include "log.h"
#include "db.h"
#include "questmanager.h"
#include "login_sim.h"
#include "fishing.h"
#include "TrafficProfiler.h"
#include "priv_manager.h"
#include "castle.h"
#include "dev_log.h"
#include "HackShield_Impl.h"

#ifndef __WIN32__
	#include "limit_time.h"
#endif

extern time_t get_global_time();
extern bool g_bNoPasspod;

bool IsEmptyAdminPage()
{
	return g_stAdminPageIP.empty();
}

bool IsAdminPage(const char * ip)
{
	for (size_t n = 0; n < g_stAdminPageIP.size(); ++n)
	{
		if (g_stAdminPageIP[n] == ip)
			return 1; 
	}	
	return 0;
}

void ClearAdminPages()
{
	for (size_t n = 0; n < g_stAdminPageIP.size(); ++n)
		g_stAdminPageIP[n].clear();

	g_stAdminPageIP.clear();
}

CInputProcessor::CInputProcessor() : m_pPacketInfo(NULL), m_iBufferLeft(0)
{
	if (!m_pPacketInfo)
		BindPacketInfo(&m_packetInfoCG);
}

void CInputProcessor::BindPacketInfo(CPacketInfo * pPacketInfo)
{
	m_pPacketInfo = pPacketInfo;
}

bool CInputProcessor::Process(LPDESC lpDesc, const void * c_pvOrig, int iBytes, int & r_iBytesProceed)
{
	const char * c_pData = (const char *) c_pvOrig;

	BYTE	bLastHeader = 0;
	int		iLastPacketLen = 0;
	int		iPacketLen;

	if (!m_pPacketInfo)
	{
		sys_err("No packet info has been binded to");
		return true;
	}

	for (m_iBufferLeft = iBytes; m_iBufferLeft > 0;)
	{
		BYTE bHeader = (BYTE) *(c_pData);
		const char * c_pszName;

		if (bHeader == 0) // 암호화 처리가 있으므로 0번 헤더는 스킵한다.
			iPacketLen = 1;
		else if (!m_pPacketInfo->Get(bHeader, &iPacketLen, &c_pszName))
		{
			sys_err("UNKNOWN HEADER: %d, LAST HEADER: %d(%d), REMAIN BYTES: %d, fd: %d",
					bHeader, bLastHeader, iLastPacketLen, m_iBufferLeft, lpDesc->GetSocket());
			//printdata((BYTE *) c_pvOrig, m_iBufferLeft);
			lpDesc->SetPhase(PHASE_CLOSE);
			return true;
		}

		if (m_iBufferLeft < iPacketLen)
			return true;

		if (bHeader)
		{
			if (test_server && bHeader != HEADER_CG_MOVE)
				sys_log(0, "Packet Analyze [Header %d][bufferLeft %d] ", bHeader, m_iBufferLeft);

			m_pPacketInfo->Start();

			int iExtraPacketSize = Analyze(lpDesc, bHeader, c_pData);

			if (iExtraPacketSize < 0)
				return true;

			iPacketLen += iExtraPacketSize;
			lpDesc->Log("%s %d", c_pszName, iPacketLen);
			m_pPacketInfo->End();
		}

		// TRAFFIC_PROFILER
		if (g_bTrafficProfileOn)
			TrafficProfiler::instance().Report(TrafficProfiler::IODIR_INPUT, bHeader, iPacketLen);
		// END_OF_TRAFFIC_PROFILER

		if (bHeader == HEADER_CG_PONG)
			sys_log(0, "PONG! %u %u", m_pPacketInfo->IsSequence(bHeader), *(BYTE *) (c_pData + iPacketLen - sizeof(BYTE)));

		if (m_pPacketInfo->IsSequence(bHeader))
		{
			BYTE bSeq = lpDesc->GetSequence();
			BYTE bSeqReceived = *(BYTE *) (c_pData + iPacketLen - sizeof(BYTE));

			if (bSeq != bSeqReceived)
			{
				sys_err("SEQUENCE %x mismatch 0x%x != 0x%x header %u", get_pointer(lpDesc), bSeq, bSeqReceived, bHeader);

				LPCHARACTER	ch = lpDesc->GetCharacter();

				char buf[1024];
				int	offset, len;

				offset = snprintf(buf, sizeof(buf), "SEQUENCE_LOG [%s]-------------\n", ch ? ch->GetName() : "UNKNOWN");

				if (offset < 0 || offset >= (int) sizeof(buf))
					offset = sizeof(buf) - 1;

				for (size_t i = 0; i < lpDesc->m_seq_vector.size(); ++i)
				{
					len = snprintf(buf + offset, sizeof(buf) - offset, "\t[%03d : 0x%x]\n",
							lpDesc->m_seq_vector[i].hdr,
							lpDesc->m_seq_vector[i].seq);

					if (len < 0 || len >= (int) sizeof(buf) - offset)
						offset += (sizeof(buf) - offset) - 1;
					else
						offset += len;
				}

				snprintf(buf + offset, sizeof(buf) - offset, "\t[%03d : 0x%x]\n", bHeader, bSeq);
				sys_err("%s", buf);

				lpDesc->SetPhase(PHASE_CLOSE);
				return true;
			}
			else
			{
				lpDesc->push_seq(bHeader, bSeq);
				lpDesc->SetNextSequence();
				//sys_err("SEQUENCE %x match %u next %u header %u", lpDesc, bSeq, lpDesc->GetSequence(), bHeader);
			}
		}

		c_pData	+= iPacketLen;
		m_iBufferLeft -= iPacketLen;
		r_iBytesProceed += iPacketLen;

		iLastPacketLen = iPacketLen;
		bLastHeader	= bHeader;

		if (GetType() != lpDesc->GetInputProcessor()->GetType())
			return false;
	}

	return true;
}

void CInputProcessor::Pong(LPDESC d)
{
	d->SetPong(true);

#ifdef ENABLE_LIMIT_TIME
#endif
}

void CInputProcessor::Handshake(LPDESC d, const char * c_pData)
{
	TPacketCGHandshake * p = (TPacketCGHandshake *) c_pData;

	if (d->GetHandshake() != p->dwHandshake)
	{
		sys_err("Invalid Handshake on %d", d->GetSocket());
		d->SetPhase(PHASE_CLOSE);
	}
	else
	{
		if (d->IsPhase(PHASE_HANDSHAKE))
		{
			if (d->HandshakeProcess(p->dwTime, p->lDelta, false))
			{
#ifdef _IMPROVED_PACKET_ENCRYPTION_
				d->SendKeyAgreement();
#else
				if (g_bAuthServer)
					d->SetPhase(PHASE_AUTH);
				else
					d->SetPhase(PHASE_LOGIN);
#endif // #ifdef _IMPROVED_PACKET_ENCRYPTION_
			}
		}
		else
			d->HandshakeProcess(p->dwTime, p->lDelta, true);
	}
}

void CInputProcessor::Version(LPCHARACTER ch, const char* c_pData)
{
	if (!ch)
		return;

	TPacketCGClientVersion * p = (TPacketCGClientVersion *) c_pData;
	sys_log(0, "VERSION: %s %s %s", ch->GetName(), p->timestamp, p->filename);
	ch->GetDesc()->SetClientVersion(p->timestamp);
}

void LoginFailure(LPDESC d, const char * c_pszStatus)
{
	if (!d)
		return;

	TPacketGCLoginFailure failurePacket;

	failurePacket.header = HEADER_GC_LOGIN_FAILURE;
	strlcpy(failurePacket.szStatus, c_pszStatus, sizeof(failurePacket.szStatus));

	d->Packet(&failurePacket, sizeof(failurePacket));
}

CInputHandshake::CInputHandshake()
{
	CPacketInfoCG * pkPacketInfo = M2_NEW CPacketInfoCG;
	pkPacketInfo->SetSequence(HEADER_CG_PONG, false);

	m_pMainPacketInfo = m_pPacketInfo;
	BindPacketInfo(pkPacketInfo);
}

CInputHandshake::~CInputHandshake()
{
	if( NULL != m_pPacketInfo )
	{
		M2_DELETE(m_pPacketInfo);
		m_pPacketInfo = NULL;
	}
}


std::map<DWORD, CLoginSim *> g_sim;
std::map<DWORD, CLoginSim *> g_simByPID;
std::vector<TPlayerTable> g_vec_save;

// BLOCK_CHAT
ACMD(do_block_chat);
// END_OF_BLOCK_CHAT

int CInputHandshake::Analyze(LPDESC d, BYTE bHeader, const char * c_pData)
{
	if (bHeader == 10) // 엔터는 무시
		return 0;

	if (bHeader == HEADER_CG_TEXT)
	{
		++c_pData;
		const char * c_pSep;

		if (!(c_pSep = strchr(c_pData, '\n')))	// \n을 찾는다.
			return -1;

		if (*(c_pSep - 1) == '\r')
			--c_pSep;

		std::string stResult;
		std::string stBuf;
		stBuf.assign(c_pData, 0, c_pSep - c_pData);

		sys_log(0, "SOCKET_CMD: FROM(%s) CMD(%s)", d->GetHostName(), stBuf.c_str());

		if (!stBuf.compare("IS_SERVER_UP"))
		{
			if (g_bNoMoreClient)
				stResult = "NO";
			else
				stResult = "YES";
		}
		else if (!stBuf.compare("IS_PASSPOD_UP"))
		{
			if (g_bNoPasspod)
				stResult = "NO";
			else
				stResult = "YES";
		}
		//else if (!stBuf.compare("SHOWMETHEMONEY"))
		else if (stBuf == g_stAdminPagePassword)
		{
			if (!IsEmptyAdminPage())
			{
				if (!IsAdminPage(inet_ntoa(d->GetAddr().sin_addr)))
				{
					char szTmp[64];
					snprintf(szTmp, sizeof(szTmp), "WEBADMIN : Wrong Connector : %s", inet_ntoa(d->GetAddr().sin_addr));
					stResult += szTmp;
				}
				else
				{
					d->SetAdminMode();
					stResult = "UNKNOWN";
				}
			}
			else
			{
				d->SetAdminMode();
				stResult = "UNKNOWN";
			}
		}
		else if (!stBuf.compare("USER_COUNT"))
		{
			char szTmp[64];

			if (!IsEmptyAdminPage())
			{
				if (!IsAdminPage(inet_ntoa(d->GetAddr().sin_addr)))
				{
					snprintf(szTmp, sizeof(szTmp), "WEBADMIN : Wrong Connector : %s", inet_ntoa(d->GetAddr().sin_addr));
				}
				else
				{
					int iTotal;
					int * paiEmpireUserCount;
					int iLocal;
					DESC_MANAGER::instance().GetUserCount(iTotal, &paiEmpireUserCount, iLocal);
					snprintf(szTmp, sizeof(szTmp), "%d %d %d %d %d", iTotal, paiEmpireUserCount[1], paiEmpireUserCount[2], paiEmpireUserCount[3], iLocal);
				}
			}
			else
			{
				int iTotal;
				int * paiEmpireUserCount;
				int iLocal;
				DESC_MANAGER::instance().GetUserCount(iTotal, &paiEmpireUserCount, iLocal);
				snprintf(szTmp, sizeof(szTmp), "%d %d %d %d %d", iTotal, paiEmpireUserCount[1], paiEmpireUserCount[2], paiEmpireUserCount[3], iLocal);
			}
			stResult += szTmp;
		}
		else if (!stBuf.compare("CHECK_P2P_CONNECTIONS"))
		{
			std::ostringstream oss(std::ostringstream::out);
			
			oss << "P2P CONNECTION NUMBER : " << P2P_MANAGER::instance().GetDescCount() << "\n";
			std::string hostNames;
			P2P_MANAGER::Instance().GetP2PHostNames(hostNames);
			oss << hostNames;
			stResult = oss.str();
			TPacketGGCheckAwakeness packet;
			packet.bHeader = HEADER_GG_CHECK_AWAKENESS;

			P2P_MANAGER::instance().Send(&packet, sizeof(packet));
		}
		else if (!stBuf.compare("PACKET_INFO"))
		{
			m_pMainPacketInfo->Log("packet_info.txt");
			stResult = "OK";
		}
		else if (!stBuf.compare("PROFILE"))
		{
			CProfiler::instance().Log("profile.txt");
			stResult = "OK";
		}
		//gift notify delete command
		else if (!stBuf.compare(0,15,"DELETE_AWARDID "))
			{
				char szTmp[64];
				std::string msg = stBuf.substr(15,26);	// item_award의 id범위?
				
				TPacketDeleteAwardID p;
				p.dwID = (DWORD)(atoi(msg.c_str()));
				snprintf(szTmp,sizeof(szTmp),"Sent to DB cache to delete ItemAward, id: %d",p.dwID);
				//sys_log(0,"%d",p.dwID);
				// strlcpy(p.login, msg.c_str(), sizeof(p.login));
				db_clientdesc->DBPacket(HEADER_GD_DELETE_AWARDID, 0, &p, sizeof(p));
				stResult += szTmp;
			}
		else
		{
			stResult = "UNKNOWN";
			
			if (d->IsAdminMode())
			{
				// 어드민 명령들
				if (!stBuf.compare(0, 7, "NOTICE "))
				{
					std::string msg = stBuf.substr(7, 50);
					LogManager::instance().CharLog(0, 0, 0, 1, "NOTICE", msg.c_str(), d->GetHostName());
					BroadcastNotice(msg.c_str());
				}
				else if (!stBuf.compare("CLOSE_PASSPOD"))
				{
					g_bNoPasspod = true;
					stResult += "CLOSE_PASSPOD";
				}
				else if (!stBuf.compare("OPEN_PASSPOD"))
				{
					g_bNoPasspod = false;
					stResult += "OPEN_PASSPOD";
				}
				else if (!stBuf.compare("SHUTDOWN"))
				{
					LogManager::instance().CharLog(0, 0, 0, 2, "SHUTDOWN", "", d->GetHostName());
					TPacketGGShutdown p;
					p.bHeader = HEADER_GG_SHUTDOWN;
					P2P_MANAGER::instance().Send(&p, sizeof(TPacketGGShutdown));
					sys_err("Accept shutdown command from %s.", d->GetHostName());
					Shutdown(10);
				}
				else if (!stBuf.compare("SHUTDOWN_ONLY"))
				{
					LogManager::instance().CharLog(0, 0, 0, 2, "SHUTDOWN", "", d->GetHostName());
					sys_err("Accept shutdown only command from %s.", d->GetHostName());
					Shutdown(10);
				}
				else if (!stBuf.compare(0, 3, "DC "))
				{
					std::string msg = stBuf.substr(3, LOGIN_MAX_LEN);

dev_log(LOG_DEB0, "DC : '%s'", msg.c_str());

					TPacketGGDisconnect pgg;

					pgg.bHeader = HEADER_GG_DISCONNECT;
					strlcpy(pgg.szLogin, msg.c_str(), sizeof(pgg.szLogin));

					P2P_MANAGER::instance().Send(&pgg, sizeof(TPacketGGDisconnect));

					// delete login key
					{
						TPacketDC p;
						strlcpy(p.login, msg.c_str(), sizeof(p.login));
						db_clientdesc->DBPacket(HEADER_GD_DC, 0, &p, sizeof(p));
					}
				}
				else if (!stBuf.compare(0, 10, "RELOAD_CRC"))
				{
					LoadValidCRCList();

					BYTE bHeader = HEADER_GG_RELOAD_CRC_LIST;
					P2P_MANAGER::instance().Send(&bHeader, sizeof(BYTE));
					stResult = "OK";
				}
				else if (!stBuf.compare(0, 20, "CHECK_CLIENT_VERSION"))
				{
					CheckClientVersion();

					BYTE bHeader = HEADER_GG_CHECK_CLIENT_VERSION;
					P2P_MANAGER::instance().Send(&bHeader, sizeof(BYTE));
					stResult = "OK";
				}
				else if (!stBuf.compare(0, 6, "RELOAD"))
				{
					if (stBuf.size() == 6)
					{
						LoadStateUserCount();
						db_clientdesc->DBPacket(HEADER_GD_RELOAD_PROTO, 0, NULL, 0);
						DBManager::instance().LoadDBString();
					}
					else
					{
						char c = stBuf[7];

						switch (LOWER(c))
						{
							case 'u':
								LoadStateUserCount();
								break;

							case 'p':
								db_clientdesc->DBPacket(HEADER_GD_RELOAD_PROTO, 0, NULL, 0);
								break;

							case 's':
								DBManager::instance().LoadDBString();
								break;

							case 'q':
								quest::CQuestManager::instance().Reload();
								break;

							case 'f':
								fishing::Initialize();
								break;

							case 'a':
								db_clientdesc->DBPacket(HEADER_GD_RELOAD_ADMIN, 0, NULL, 0);
								sys_log(0, "Reloading admin infomation.");
								break;
						}
					}
				}
				else if (!stBuf.compare(0, 6, "EVENT "))
				{
					std::istringstream is(stBuf);
					std::string strEvent, strFlagName;
					long lValue;
					is >> strEvent >> strFlagName >> lValue;

					if (!is.fail())
					{
						sys_log(0, "EXTERNAL EVENT FLAG name %s value %d", strFlagName.c_str(), lValue);
						quest::CQuestManager::instance().RequestSetEventFlag(strFlagName, lValue);
						stResult = "EVENT FLAG CHANGE ";
						stResult += strFlagName;
					}
					else
					{
						stResult = "EVENT FLAG FAIL";
					}
				}
				// BLOCK_CHAT
				else if (!stBuf.compare(0, 11, "BLOCK_CHAT "))
				{
					std::istringstream is(stBuf);
					std::string strBlockChat, strCharName;
					long lDuration;
					is >> strBlockChat >> strCharName >> lDuration;

					if (!is.fail())
					{
						sys_log(0, "EXTERNAL BLOCK_CHAT name %s duration %d", strCharName.c_str(), lDuration);

						do_block_chat(NULL, const_cast<char*>(stBuf.c_str() + 11), 0, 0);

						stResult = "BLOCK_CHAT ";
						stResult += strCharName;
					}
					else
					{
						stResult = "BLOCK_CHAT FAIL";
					}
				}
				// END_OF_BLOCK_CHAT
				else if (!stBuf.compare(0, 12, "PRIV_EMPIRE "))
				{
					int	empire, type, value, duration;
					std::istringstream is(stBuf);
					std::string strPrivEmpire;
					is >> strPrivEmpire >> empire >> type >> value >> duration;

					// 최대치 10배
					value = MINMAX(0, value, 1000);
					stResult = "PRIV_EMPIRE FAIL";

					if (!is.fail())
					{
						// check parameter
						if (empire < 0 || 3 < empire);
						else if (type < 1 || 4 < type);
						else if (value < 0);
						else if (duration < 0);
						else
						{
							stResult = "PRIV_EMPIRE SUCCEED";

							// 시간 단위로 변경
							duration = duration * (60 * 60);

							sys_log(0, "_give_empire_privileage(empire=%d, type=%d, value=%d, duration=%d) by web", 
									empire, type, value, duration);
							CPrivManager::instance().RequestGiveEmpirePriv(empire, type, value, duration);
						}
					}
				}
				else if (!stBuf.compare(0, 15, "BLOCK_EXCEPTION"))
				{
					// BLOCK_EXCEPTION cmd(add=1, del=2) login
					std::istringstream is(stBuf);
					std::string	dummy_string;
					std::string	login_string;
					int			cmd;

					is >> dummy_string >> cmd >> login_string;

					sys_log(0, "block_exception %s:%d", login_string.c_str(), cmd);
					DBManager::instance().RequestBlockException(login_string.c_str(), cmd);
					stResult = "BLOCK_EXCEPTION_YES";
				}
			}
		}

		sys_log(1, "TEXT %s RESULT %s", stBuf.c_str(), stResult.c_str());
		stResult += "\n";
		d->Packet(stResult.c_str(), stResult.length());
		return (c_pSep - c_pData) + 1;
	}
	else if (bHeader == HEADER_CG_MARK_LOGIN)
	{
		if (!guild_mark_server)
		{
			// 끊어버려! - 마크 서버가 아닌데 마크를 요청하려고?
			sys_err("Guild Mark login requested but i'm not a mark server!");
			d->SetPhase(PHASE_CLOSE);
			return 0;
		}

		// 무조건 인증 --;
		sys_log(0, "MARK_SERVER: Login");
		d->SetPhase(PHASE_LOGIN);
		return 0;
	}
	else if (bHeader == HEADER_CG_STATE_CHECKER)
	{
		if (d->isChannelStatusRequested()) {
			return 0;
		}
		d->SetChannelStatusRequested(true);
		db_clientdesc->DBPacket(HEADER_GD_REQUEST_CHANNELSTATUS, d->GetHandle(), NULL, 0);
	}
	else if (bHeader == HEADER_CG_PONG)
		Pong(d);
	else if (bHeader == HEADER_CG_HANDSHAKE)
		Handshake(d, c_pData);
#ifdef _IMPROVED_PACKET_ENCRYPTION_
	else if (bHeader == HEADER_CG_KEY_AGREEMENT)
	{
		// Send out the key agreement completion packet first
		// to help client to enter encryption mode
		d->SendKeyAgreementCompleted();
		// Flush socket output before going encrypted
		d->ProcessOutput();

		TPacketKeyAgreement* p = (TPacketKeyAgreement*)c_pData;
		if (!d->IsCipherPrepared())
		{
			sys_err ("Cipher isn't prepared. %s maybe a Hacker.", inet_ntoa(d->GetAddr().sin_addr));
			d->DelayedDisconnect(5);
			return 0;
		}
		if (d->FinishHandshake(p->wAgreedLength, p->data, p->wDataLength)) {
			// Handshaking succeeded
			if (g_bAuthServer) {
				d->SetPhase(PHASE_AUTH);
			} else {
				d->SetPhase(PHASE_LOGIN);
			}
		} else {
			sys_log(0, "[CInputHandshake] Key agreement failed: al=%u dl=%u",
				p->wAgreedLength, p->wDataLength);
			d->SetPhase(PHASE_CLOSE);
		}
	}
#endif // _IMPROVED_PACKET_ENCRYPTION_
	else
		sys_err("Handshake phase does not handle packet %d (fd %d)", bHeader, d->GetSocket());

	return 0;
}


