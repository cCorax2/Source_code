#include "stdafx.h"
#include "config.h"
#include "utils.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "char.h"
#include "protocol.h"
#include "p2p.h"
#include "buffer_manager.h"
#include "guild_manager.h"
#include "db.h"

#include "party.h"

extern LPFDWATCH	main_fdw;

LPCLIENT_DESC db_clientdesc = NULL;
LPCLIENT_DESC g_pkAuthMasterDesc = NULL;
LPCLIENT_DESC g_NetmarbleDBDesc = NULL;
LPCLIENT_DESC g_TeenDesc		= NULL;
LPCLIENT_DESC g_PasspodDesc = NULL;

static const char* GetKnownClientDescName(LPCLIENT_DESC desc) {
	if (desc == db_clientdesc) {
		return "db_clientdesc";
	} else if (desc == g_pkAuthMasterDesc) {
		return "g_pkAuthMasterDesc";
	} else if (desc == g_NetmarbleDBDesc) {
		return "g_NetmarbleDBDesc";
	} else if (desc == g_TeenDesc) {
		return "g_TeenDesc";
	} else if (desc == g_PasspodDesc) {
		return "g_PasspodDesc";
	}
	return "unknown";
}

CLIENT_DESC::CLIENT_DESC()
{
	m_iPhaseWhenSucceed = 0;
	m_bRetryWhenClosed = false;
	m_LastTryToConnectTime = 0;
	m_tLastChannelStatusUpdateTime = 0;
}

CLIENT_DESC::~CLIENT_DESC()
{
}

void CLIENT_DESC::Destroy()
{
	if (m_sock == INVALID_SOCKET) {
		return;
	}

	P2P_MANAGER::instance().UnregisterConnector(this);

	if (this == db_clientdesc)
	{
		CPartyManager::instance().DeleteAllParty();
		CPartyManager::instance().DisablePCParty();
		CGuildManager::instance().StopAllGuildWar();
		DBManager::instance().StopAllBilling();
	}

	fdwatch_del_fd(m_lpFdw, m_sock);

	sys_log(0, "SYSTEM: closing client socket. DESC #%d", m_sock);

	socket_close(m_sock);
	m_sock = INVALID_SOCKET;

	// Chain up to base class Destroy()
	DESC::Destroy();
}

void CLIENT_DESC::SetRetryWhenClosed(bool b)
{
	m_bRetryWhenClosed = b;
}

bool CLIENT_DESC::Connect(int iPhaseWhenSucceed)
{
	if (iPhaseWhenSucceed != 0)
		m_iPhaseWhenSucceed = iPhaseWhenSucceed;

	if (get_global_time() - m_LastTryToConnectTime < 3)	// 3초
		return false;

	m_LastTryToConnectTime = get_global_time();

	if (m_sock != INVALID_SOCKET)
		return false;

	sys_log(0, "SYSTEM: Trying to connect to %s:%d", m_stHost.c_str(), m_wPort);

	m_sock = socket_connect(m_stHost.c_str(), m_wPort);

	if (m_sock != INVALID_SOCKET)
	{
		sys_log(0, "SYSTEM: connected to server (fd %d, ptr %p)", m_sock, this);
		fdwatch_add_fd(m_lpFdw, m_sock, this, FDW_READ, false);
		fdwatch_add_fd(m_lpFdw, m_sock, this, FDW_WRITE, false);
		SetPhase(m_iPhaseWhenSucceed);
		return true;
	}
	else
	{
		SetPhase(PHASE_CLIENT_CONNECTING);
		return false;
	}
}

void CLIENT_DESC::Setup(LPFDWATCH _fdw, const char * _host, WORD _port)
{
	// 1MB input/output buffer
	m_lpFdw = _fdw;
	m_stHost = _host;
	m_wPort = _port;

	InitializeBuffers();

	m_sock = INVALID_SOCKET;
}

void CLIENT_DESC::SetPhase(int iPhase)
{
	switch (iPhase)
	{
		case PHASE_CLIENT_CONNECTING:
			sys_log(1, "PHASE_CLIENT_DESC::CONNECTING");
			m_pInputProcessor = NULL;
			break;

		case PHASE_DBCLIENT:
			{
				sys_log(1, "PHASE_DBCLIENT");

				if (!g_bAuthServer)
				{
					static bool bSentBoot = false;

					if (!bSentBoot)
					{
						bSentBoot = true;
						TPacketGDBoot p;
						p.dwItemIDRange[0] = 0;
						p.dwItemIDRange[1] = 0;
						memcpy(p.szIP, g_szPublicIP, 16);
						DBPacket(HEADER_GD_BOOT, 0, &p, sizeof(p));
					}
				}

				TEMP_BUFFER buf;

				TPacketGDSetup p;

				memset(&p, 0, sizeof(p));
				strlcpy(p.szPublicIP, g_szPublicIP, sizeof(p.szPublicIP));

				if (!g_bAuthServer)
				{
					p.bChannel	= g_bChannel;
					p.wListenPort = mother_port;
					p.wP2PPort	= p2p_port;
					p.bAuthServer = false;
					map_allow_copy(p.alMaps, 32);

					const DESC_MANAGER::DESC_SET & c_set = DESC_MANAGER::instance().GetClientSet();
					DESC_MANAGER::DESC_SET::const_iterator it;

					for (it = c_set.begin(); it != c_set.end(); ++it)
					{
						LPDESC d = *it;

						if (d->GetAccountTable().id != 0)
							++p.dwLoginCount;
					}

					buf.write(&p, sizeof(p));

					if (p.dwLoginCount)
					{
						TPacketLoginOnSetup pck;

						for (it = c_set.begin(); it != c_set.end(); ++it)
						{
							LPDESC d = *it;

							TAccountTable & r = d->GetAccountTable();

							if (r.id != 0)
							{
								pck.dwID = r.id;
								strlcpy(pck.szLogin, r.login, sizeof(pck.szLogin));
								strlcpy(pck.szSocialID, r.social_id, sizeof(pck.szSocialID));
								strlcpy(pck.szHost, d->GetHostName(), sizeof(pck.szHost));
								pck.dwLoginKey = d->GetLoginKey();
#ifndef _IMPROVED_PACKET_ENCRYPTION_
								thecore_memcpy(pck.adwClientKey, d->GetDecryptionKey(), 16);
#endif

								buf.write(&pck, sizeof(TPacketLoginOnSetup));
							}
						}
					}

					sys_log(0, "DB_SETUP current user %d size %d", p.dwLoginCount, buf.size());

					// 파티를 처리할 수 있게 됨.
					CPartyManager::instance().EnablePCParty();
					//CPartyManager::instance().SendPartyToDB();
				}
				else
				{
					p.bAuthServer = true;
					buf.write(&p, sizeof(p));
				}

				DBPacket(HEADER_GD_SETUP, 0, buf.read_peek(), buf.size());
				m_pInputProcessor = &m_inputDB;
			}
			break;

		case PHASE_P2P:
			sys_log(1, "PHASE_P2P");
			
			if (m_lpInputBuffer)
				buffer_reset(m_lpInputBuffer);

			if (m_lpOutputBuffer)
				buffer_reset(m_lpOutputBuffer);

			m_pInputProcessor = &m_inputP2P;
			break;

		case PHASE_CLOSE:
			m_pInputProcessor = NULL;
			break;

		case PHASE_TEEN:
			m_inputTeen.SetStep(0);
			m_pInputProcessor = &m_inputTeen;
			break;

	}

	m_iPhase = iPhase;
}

void CLIENT_DESC::DBPacketHeader(BYTE bHeader, DWORD dwHandle, DWORD dwSize)
{
	buffer_write(m_lpOutputBuffer, encode_byte(bHeader), sizeof(BYTE));
	buffer_write(m_lpOutputBuffer, encode_4bytes(dwHandle), sizeof(DWORD));
	buffer_write(m_lpOutputBuffer, encode_4bytes(dwSize), sizeof(DWORD));
}

void CLIENT_DESC::DBPacket(BYTE bHeader, DWORD dwHandle, const void * c_pvData, DWORD dwSize)
{
	if (m_sock == INVALID_SOCKET) {
		sys_log(0, "CLIENT_DESC [%s] trying DBPacket() while not connected",
			GetKnownClientDescName(this));
		return;
	}
	sys_log(1, "DB_PACKET: header %d handle %d size %d buffer_size %d", bHeader, dwHandle, dwSize, buffer_size(m_lpOutputBuffer));
	DBPacketHeader(bHeader, dwHandle, dwSize);

	if (c_pvData)
		buffer_write(m_lpOutputBuffer, c_pvData, dwSize);
}

void CLIENT_DESC::Packet(const void * c_pvData, int iSize)
{
	if (m_sock == INVALID_SOCKET) {
		sys_log(0, "CLIENT_DESC [%s] trying Packet() while not connected",
			GetKnownClientDescName(this));
		return;
	}
	buffer_write(m_lpOutputBuffer, c_pvData, iSize);
}

bool CLIENT_DESC::IsRetryWhenClosed()
{
	return (0 == thecore_is_shutdowned() && m_bRetryWhenClosed);
}

void CLIENT_DESC::Update(DWORD t)
{
	if (!g_bAuthServer) {
		UpdateChannelStatus(t, false);
	}
}

void CLIENT_DESC::UpdateChannelStatus(DWORD t, bool fForce)
{
	enum {
		CHANNELSTATUS_UPDATE_PERIOD = 5*60*1000,	// 5분마다
	};
	if (fForce || m_tLastChannelStatusUpdateTime+CHANNELSTATUS_UPDATE_PERIOD < t) {
		int iTotal; 
		int * paiEmpireUserCount;
		int iLocal;
		DESC_MANAGER::instance().GetUserCount(iTotal, &paiEmpireUserCount, iLocal);

		TChannelStatus channelStatus;
		channelStatus.nPort = mother_port;

		if (g_bNoMoreClient) channelStatus.bStatus = 0;
		else channelStatus.bStatus = iTotal > g_iFullUserCount ? 3 : iTotal > g_iBusyUserCount ? 2 : 1;

		DBPacket(HEADER_GD_UPDATE_CHANNELSTATUS, 0, &channelStatus, sizeof(channelStatus));
		m_tLastChannelStatusUpdateTime = t;
	}
}

void CLIENT_DESC::Reset()
{
	// Backup connection target info
	LPFDWATCH fdw = m_lpFdw;
	std::string host = m_stHost;
	WORD port = m_wPort;

	Destroy();
	Initialize();

	// Restore connection target info
	m_lpFdw = fdw;
	m_stHost = host;
	m_wPort = port;

	InitializeBuffers();
}

void CLIENT_DESC::InitializeBuffers()
{
	m_lpOutputBuffer = buffer_new(1024 * 1024);
	m_lpInputBuffer = buffer_new(1024 * 1024);
	m_iMinInputBufferLen = 1024 * 1024;
}
