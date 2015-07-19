#include "stdafx.h"
#include "config.h"
#include "utils.h"
#include "desc.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "char.h"
#include "protocol.h"
#include "packet.h"
#include "messenger_manager.h"
#include "sectree_manager.h"
#include "p2p.h"
#include "buffer_manager.h"
#include "sequence.h"
#include "guild.h"
#include "guild_manager.h"
#include "TrafficProfiler.h"
#include "locale_service.h"
#include "HackShield.h"
#include "log.h"

extern int max_bytes_written;
extern int current_bytes_written;
extern int total_bytes_written;

DESC::DESC()
{
	Initialize();
}

DESC::~DESC()
{
}

void DESC::Initialize()
{
	m_bDestroyed = false;

	m_pInputProcessor = NULL;
	m_lpFdw = NULL;
	m_sock = INVALID_SOCKET;
	m_iPhase = PHASE_CLOSE;
	m_dwHandle = 0;

	m_wPort = 0;
	m_LastTryToConnectTime = 0;

	m_lpInputBuffer = NULL;
	m_iMinInputBufferLen = 0;

	m_dwHandshake = 0;
	m_dwHandshakeSentTime = 0;
	m_iHandshakeRetry = 0;
	m_dwClientTime = 0;
	m_bHandshaking = false;

	m_lpBufferedOutputBuffer = NULL;
	m_lpOutputBuffer = NULL;

	m_pkPingEvent = NULL;
	m_lpCharacter = NULL;
	memset( &m_accountTable, 0, sizeof(m_accountTable) );

	memset( &m_SockAddr, 0, sizeof(m_SockAddr) );
	memset( &m_UDPSockAddr, 0, sizeof(m_UDPSockAddr) );

	m_pLogFile = NULL;

#ifndef _IMPROVED_PACKET_ENCRYPTION_
	m_bEncrypted = false;
#endif

	m_wP2PPort = 0;
	m_bP2PChannel = 0;

	m_bAdminMode = false;
	m_bPong = true;
	m_bChannelStatusRequested = false;

	m_iCurrentSequence = 0;

	m_dwMatrixRows = m_dwMatrixCols = 0;
	m_bMatrixTryCount = 0;

	m_pkLoginKey = NULL;
	m_dwLoginKey = 0;
	m_dwPanamaKey = 0;

#ifndef _IMPROVED_PACKET_ENCRYPTION_
	memset( m_adwDecryptionKey, 0, sizeof(m_adwDecryptionKey) );
	memset( m_adwEncryptionKey, 0, sizeof(m_adwEncryptionKey) );
#endif

	m_bCRCMagicCubeIdx = 0;
	m_dwProcCRC = 0;
	m_dwFileCRC = 0;
	m_bHackCRCQuery = 0;

	m_dwBillingExpireSecond = 0;

	m_outtime = 0;
	m_playtime = 0;
	m_offtime = 0;

	m_pkDisconnectEvent = NULL;

	m_seq_vector.clear();
}

void DESC::Destroy()
{
	if (m_bDestroyed) {
		return;
	}
	m_bDestroyed = true;

	if (m_pkLoginKey)
		m_pkLoginKey->Expire();

	if (GetAccountTable().id)
		DESC_MANAGER::instance().DisconnectAccount(GetAccountTable().login);

	if (m_pLogFile)
	{
		fclose(m_pLogFile);
		m_pLogFile = NULL;
	}

	if (m_lpCharacter)
	{
		m_lpCharacter->Disconnect("DESC::~DESC");
		m_lpCharacter = NULL;
	}

	SAFE_BUFFER_DELETE(m_lpOutputBuffer);
	SAFE_BUFFER_DELETE(m_lpInputBuffer);

	event_cancel(&m_pkPingEvent);
	event_cancel(&m_pkDisconnectEvent);

	if (!g_bAuthServer)
	{
		if (m_accountTable.login[0] && m_accountTable.passwd[0])
		{
			TLogoutPacket pack;

			strlcpy(pack.login, m_accountTable.login, sizeof(pack.login));
			strlcpy(pack.passwd, m_accountTable.passwd, sizeof(pack.passwd));

			db_clientdesc->DBPacket(HEADER_GD_LOGOUT, m_dwHandle, &pack, sizeof(TLogoutPacket));
		}
	}

	if (m_sock != INVALID_SOCKET)
	{
		sys_log(0, "SYSTEM: closing socket. DESC #%d", m_sock);
		Log("SYSTEM: closing socket. DESC #%d", m_sock);
		fdwatch_del_fd(m_lpFdw, m_sock);

#ifdef _IMPROVED_PACKET_ENCRYPTION_
		cipher_.CleanUp();
#endif

		socket_close(m_sock);
		m_sock = INVALID_SOCKET;
	}

	m_seq_vector.clear();
}

EVENTFUNC(ping_event)
{
	DESC::desc_event_info* info = dynamic_cast<DESC::desc_event_info*>( event->info );

	if ( info == NULL )
	{
		sys_err( "ping_event> <Factor> Null pointer" );
		return 0;
	}

	LPDESC desc = info->desc;

	if (desc->IsAdminMode())
		return (ping_event_second_cycle);

	if (!desc->IsPong())
	{
		sys_log(0, "PING_EVENT: no pong %s", desc->GetHostName());

		desc->SetPhase(PHASE_CLOSE);

		return (ping_event_second_cycle);
	}
	else
	{
		TPacketGCPing p;
		p.header = HEADER_GC_PING;
		desc->Packet(&p, sizeof(struct packet_ping));
		desc->SetPong(false);
	}

#ifdef ENABLE_LIMIT_TIME
	if ((unsigned)get_global_time() >= GLOBAL_LIMIT_TIME)
	{
		extern void ClearAdminPages();
		ClearAdminPages();
		extern g_bShutdown;
		g_bShutdown = true;
	}
#endif

	desc->SendHandshake(get_dword_time(), 0);

	return (ping_event_second_cycle);
}

bool DESC::IsPong()
{
	return m_bPong;
}

void DESC::SetPong(bool b)
{
	m_bPong = b;
}

bool DESC::Setup(LPFDWATCH _fdw, socket_t _fd, const struct sockaddr_in & c_rSockAddr, DWORD _handle, DWORD _handshake)
{
	m_lpFdw		= _fdw;
	m_sock		= _fd;

	m_stHost		= inet_ntoa(c_rSockAddr.sin_addr);
	m_wPort			= c_rSockAddr.sin_port;
	m_dwHandle		= _handle;

	//if (LC_IsEurope() == true || LC_IsNewCIBN())
	//	m_lpOutputBuffer = buffer_new(DEFAULT_PACKET_BUFFER_SIZE * 2);
	//else
	//NOTE: 이걸 나라별로 다르게 잡아야할 이유가 있나?
	m_lpOutputBuffer = buffer_new(DEFAULT_PACKET_BUFFER_SIZE * 2);

	m_iMinInputBufferLen = MAX_INPUT_LEN >> 1;
	m_lpInputBuffer = buffer_new(MAX_INPUT_LEN);

	m_SockAddr = c_rSockAddr;

	fdwatch_add_fd(m_lpFdw, m_sock, this, FDW_READ, false);

	// Ping Event 
	desc_event_info* info = AllocEventInfo<desc_event_info>();

	info->desc = this;
	assert(m_pkPingEvent == NULL);

	m_pkPingEvent = event_create(ping_event, info, ping_event_second_cycle);

#ifndef _IMPROVED_PACKET_ENCRYPTION_
	if (LC_IsEurope())	
	{
		thecore_memcpy(m_adwEncryptionKey, "1234abcd5678efgh", sizeof(DWORD) * 4);
		thecore_memcpy(m_adwDecryptionKey, "1234abcd5678efgh", sizeof(DWORD) * 4);
	}
	else
	{
		thecore_memcpy(m_adwEncryptionKey, "testtesttesttest", sizeof(DWORD) * 4);
		thecore_memcpy(m_adwDecryptionKey, "testtesttesttest", sizeof(DWORD) * 4);
	}
#endif // _IMPROVED_PACKET_ENCRYPTION_

	// Set Phase to handshake
	SetPhase(PHASE_HANDSHAKE);
	StartHandshake(_handshake);

	sys_log(0, "SYSTEM: new connection from [%s] fd: %d handshake %u output input_len %d, ptr %p",
			m_stHost.c_str(), m_sock, m_dwHandshake, buffer_size(m_lpInputBuffer), this);

	Log("SYSTEM: new connection from [%s] fd: %d handshake %u ptr %p", m_stHost.c_str(), m_sock, m_dwHandshake, this);
	return true;
}

int DESC::ProcessInput()
{
	ssize_t bytes_read;

	if (!m_lpInputBuffer)
	{
		sys_err("DESC::ProcessInput : nil input buffer");
		return -1;
	}

	buffer_adjust_size(m_lpInputBuffer, m_iMinInputBufferLen);
	bytes_read = socket_read(m_sock, (char *) buffer_write_peek(m_lpInputBuffer), buffer_has_space(m_lpInputBuffer));

	if (bytes_read < 0)
		return -1;
	else if (bytes_read == 0)
		return 0;

	buffer_write_proceed(m_lpInputBuffer, bytes_read);

	if (!m_pInputProcessor)
		sys_err("no input processor");
#ifdef _IMPROVED_PACKET_ENCRYPTION_
	else
	{
		if (cipher_.activated()) {
			cipher_.Decrypt(const_cast<void*>(buffer_read_peek(m_lpInputBuffer)), buffer_size(m_lpInputBuffer));
		}

		int iBytesProceed = 0;

		// false가 리턴 되면 다른 phase로 바뀐 것이므로 다시 프로세스로 돌입한다!
		while (!m_pInputProcessor->Process(this, buffer_read_peek(m_lpInputBuffer), buffer_size(m_lpInputBuffer), iBytesProceed))
		{
			buffer_read_proceed(m_lpInputBuffer, iBytesProceed);
			iBytesProceed = 0;
		}

		buffer_read_proceed(m_lpInputBuffer, iBytesProceed);
	}
#else
	else if (!m_bEncrypted)
	{
		int iBytesProceed = 0;

		// false가 리턴 되면 다른 phase로 바뀐 것이므로 다시 프로세스로 돌입한다!
		while (!m_pInputProcessor->Process(this, buffer_read_peek(m_lpInputBuffer), buffer_size(m_lpInputBuffer), iBytesProceed))
		{
			buffer_read_proceed(m_lpInputBuffer, iBytesProceed);
			iBytesProceed = 0;
		}

		buffer_read_proceed(m_lpInputBuffer, iBytesProceed);
	}
	else
	{
		int iSizeBuffer = buffer_size(m_lpInputBuffer);

		// 8바이트 단위로만 처리한다. 8바이트 단위에 부족하면 잘못된 암호화 버퍼를 복호화
		// 할 가능성이 있으므로 짤라서 처리하기로 한다.
		if (iSizeBuffer & 7) // & 7은 % 8과 같다. 2의 승수에서만 가능
			iSizeBuffer -= iSizeBuffer & 7;

		if (iSizeBuffer > 0)
		{
			TEMP_BUFFER	tempbuf;
			LPBUFFER lpBufferDecrypt = tempbuf.getptr();
			buffer_adjust_size(lpBufferDecrypt, iSizeBuffer);

			int iSizeAfter = TEA_Decrypt((DWORD *) buffer_write_peek(lpBufferDecrypt),
					(DWORD *) buffer_read_peek(m_lpInputBuffer),
					GetDecryptionKey(),
					iSizeBuffer);

			buffer_write_proceed(lpBufferDecrypt, iSizeAfter);

			int iBytesProceed = 0;

			// false가 리턴 되면 다른 phase로 바뀐 것이므로 다시 프로세스로 돌입한다!
			while (!m_pInputProcessor->Process(this, buffer_read_peek(lpBufferDecrypt), buffer_size(lpBufferDecrypt), iBytesProceed))
			{
				if (iBytesProceed > iSizeBuffer)
				{
					buffer_read_proceed(m_lpInputBuffer, iSizeBuffer);
					iSizeBuffer = 0;
					iBytesProceed = 0;
					break;
				}

				buffer_read_proceed(m_lpInputBuffer, iBytesProceed);
				iSizeBuffer -= iBytesProceed;

				buffer_read_proceed(lpBufferDecrypt, iBytesProceed);
				iBytesProceed = 0;
			}

			buffer_read_proceed(m_lpInputBuffer, iBytesProceed);
		}
	}
#endif // _IMPROVED_PACKET_ENCRYPTION_

	return (bytes_read);
}

int DESC::ProcessOutput()
{
	if (buffer_size(m_lpOutputBuffer) <= 0)
		return 0;

	int buffer_left = fdwatch_get_buffer_size(m_lpFdw, m_sock);

	if (buffer_left <= 0)
		return 0;

	int bytes_to_write = MIN(buffer_left, buffer_size(m_lpOutputBuffer));

	if (bytes_to_write == 0)
		return 0;

	int result = socket_write(m_sock, (const char *) buffer_read_peek(m_lpOutputBuffer), bytes_to_write);

	if (result == 0)
	{
		//sys_log(0, "%d bytes written to %s first %u", bytes_to_write, GetHostName(), *(BYTE *) buffer_read_peek(m_lpOutputBuffer));
		//Log("%d bytes written", bytes_to_write);
		max_bytes_written = MAX(bytes_to_write, max_bytes_written);

		total_bytes_written += bytes_to_write;
		current_bytes_written += bytes_to_write;

		buffer_read_proceed(m_lpOutputBuffer, bytes_to_write);

		if (buffer_size(m_lpOutputBuffer) != 0)
			fdwatch_add_fd(m_lpFdw, m_sock, this, FDW_WRITE, true);
	}

	return (result);
}

void DESC::BufferedPacket(const void * c_pvData, int iSize)
{
	if (m_iPhase == PHASE_CLOSE)
		return;

	if (!m_lpBufferedOutputBuffer)
		m_lpBufferedOutputBuffer = buffer_new(MAX(1024, iSize));

	buffer_write(m_lpBufferedOutputBuffer, c_pvData, iSize);
}

void DESC::Packet(const void * c_pvData, int iSize)
{
	assert(iSize > 0);

	if (m_iPhase == PHASE_CLOSE) // 끊는 상태면 보내지 않는다.
		return;

	if (m_stRelayName.length() != 0)
	{
		// Relay 패킷은 암호화하지 않는다.
		TPacketGGRelay p;

		p.bHeader = HEADER_GG_RELAY;
		strlcpy(p.szName, m_stRelayName.c_str(), sizeof(p.szName));
		p.lSize = iSize;

		if (!packet_encode(m_lpOutputBuffer, &p, sizeof(p)))
		{
			m_iPhase = PHASE_CLOSE;
			return;
		}

		m_stRelayName.clear();

		if (!packet_encode(m_lpOutputBuffer, c_pvData, iSize))
		{
			m_iPhase = PHASE_CLOSE;
			return;
		}
	}
	else
	{
		if (m_lpBufferedOutputBuffer)
		{
			buffer_write(m_lpBufferedOutputBuffer, c_pvData, iSize);

			c_pvData = buffer_read_peek(m_lpBufferedOutputBuffer);
			iSize = buffer_size(m_lpBufferedOutputBuffer);
		}

		// TRAFFIC_PROFILE
		if (g_bTrafficProfileOn)
			TrafficProfiler::instance().Report(TrafficProfiler::IODIR_OUTPUT, *(BYTE *) c_pvData, iSize);
		// END_OF_TRAFFIC_PROFILER

#ifdef _IMPROVED_PACKET_ENCRYPTION_
		void* buf = buffer_write_peek(m_lpOutputBuffer);

		
		if (packet_encode(m_lpOutputBuffer, c_pvData, iSize))
		{
			if (cipher_.activated()) {
				cipher_.Encrypt(buf, iSize);
			}
		}
		else
		{
			m_iPhase = PHASE_CLOSE;
		}
#else
		if (!m_bEncrypted)
		{
			if (!packet_encode(m_lpOutputBuffer, c_pvData, iSize))
			{
				m_iPhase = PHASE_CLOSE;
			}
		}
		else
		{
			if (buffer_has_space(m_lpOutputBuffer) < iSize + 8)
			{
				sys_err("desc buffer mem_size overflow. memsize(%u) write_pos(%u) iSize(%d)", 
						m_lpOutputBuffer->mem_size, m_lpOutputBuffer->write_point_pos, iSize);

				m_iPhase = PHASE_CLOSE;
			}
			else
			{
				// 암호화에 필요한 충분한 버퍼 크기를 확보한다.
				/* buffer_adjust_size(m_lpOutputBuffer, iSize + 8); */
				DWORD * pdwWritePoint = (DWORD *) buffer_write_peek(m_lpOutputBuffer);

				if (packet_encode(m_lpOutputBuffer, c_pvData, iSize))
				{
					int iSize2 = TEA_Encrypt(pdwWritePoint, pdwWritePoint, GetEncryptionKey(), iSize);

					if (iSize2 > iSize)
						buffer_write_proceed(m_lpOutputBuffer, iSize2 - iSize);
				}
			}
		}
#endif // _IMPROVED_PACKET_ENCRYPTION_

		SAFE_BUFFER_DELETE(m_lpBufferedOutputBuffer);
	}

	//sys_log(0, "%d bytes written (first byte %d)", iSize, *(BYTE *) c_pvData);
	if (m_iPhase != PHASE_CLOSE)
		fdwatch_add_fd(m_lpFdw, m_sock, this, FDW_WRITE, true);
}

void DESC::LargePacket(const void * c_pvData, int iSize)
{
	buffer_adjust_size(m_lpOutputBuffer, iSize);
	sys_log(0, "LargePacket Size %d", iSize, buffer_size(m_lpOutputBuffer));

	Packet(c_pvData, iSize);
}

void DESC::SetPhase(int _phase)
{
	m_iPhase = _phase;

	TPacketGCPhase pack;
	pack.header = HEADER_GC_PHASE;
	pack.phase = _phase;
	Packet(&pack, sizeof(TPacketGCPhase));

	switch (m_iPhase)
	{
		case PHASE_CLOSE:
			// 메신저가 캐릭터단위가 되면서 삭제
			//MessengerManager::instance().Logout(GetAccountTable().login);
			m_pInputProcessor = &m_inputClose;
			break;

		case PHASE_HANDSHAKE:
			m_pInputProcessor = &m_inputHandshake;
			break;

		case PHASE_SELECT:
			// 메신저가 캐릭터단위가 되면서 삭제
			//MessengerManager::instance().Logout(GetAccountTable().login); // 의도적으로 break 안검
		case PHASE_LOGIN:
		case PHASE_LOADING:
#ifndef _IMPROVED_PACKET_ENCRYPTION_
			m_bEncrypted = true;
#endif
			m_pInputProcessor = &m_inputLogin;
			break;

		case PHASE_GAME:
		case PHASE_DEAD:
#ifndef _IMPROVED_PACKET_ENCRYPTION_
			m_bEncrypted = true;
#endif
			m_pInputProcessor = &m_inputMain;
			break;

		case PHASE_AUTH:
#ifndef _IMPROVED_PACKET_ENCRYPTION_
			m_bEncrypted = true;
#endif
			m_pInputProcessor = &m_inputAuth;
			sys_log(0, "AUTH_PHASE %p", this);
			break;
	}
}

void DESC::BindAccountTable(TAccountTable * pAccountTable)
{
	assert(pAccountTable != NULL);
	thecore_memcpy(&m_accountTable, pAccountTable, sizeof(TAccountTable));
	DESC_MANAGER::instance().ConnectAccount(m_accountTable.login, this);
}

void DESC::UDPGrant(const struct sockaddr_in & c_rSockAddr)
{
	m_UDPSockAddr = c_rSockAddr;

	TPacketGCBindUDP pack;

	pack.header	= HEADER_GC_BINDUDP;
	pack.addr	= m_UDPSockAddr.sin_addr.s_addr;
	pack.port	= m_UDPSockAddr.sin_port;

	Packet(&pack, sizeof(TPacketGCBindUDP));
}

void DESC::Log(const char * format, ...)
{
	if (!m_pLogFile)
		return;

	va_list args;

	time_t ct = get_global_time();
	struct tm tm = *localtime(&ct);

	fprintf(m_pLogFile,
			"%02d %02d %02d:%02d:%02d | ",
			tm.tm_mon + 1,
			tm.tm_mday,
			tm.tm_hour,
			tm.tm_min,
			tm.tm_sec);

	va_start(args, format);
	vfprintf(m_pLogFile, format, args);
	va_end(args);

	fputs("\n", m_pLogFile);

	fflush(m_pLogFile);
}

void DESC::StartHandshake(DWORD _handshake)
{
	// Handshake
	m_dwHandshake = _handshake;

	SendHandshake(get_dword_time(), 0);

	m_iHandshakeRetry = 0;
}

void DESC::SendHandshake(DWORD dwCurTime, long lNewDelta)
{
	TPacketGCHandshake pack;

	pack.bHeader		= HEADER_GC_HANDSHAKE;
	pack.dwHandshake	= m_dwHandshake;
	pack.dwTime			= dwCurTime;
	pack.lDelta			= lNewDelta;

	Packet(&pack, sizeof(TPacketGCHandshake));

	m_dwHandshakeSentTime = dwCurTime;
	m_bHandshaking = true;
}

bool DESC::HandshakeProcess(DWORD dwTime, long lDelta, bool bInfiniteRetry)
{
	DWORD dwCurTime = get_dword_time();

	if (lDelta < 0)
	{
		sys_err("Desc::HandshakeProcess : value error (lDelta %d, ip %s)", lDelta, m_stHost.c_str());
		return false;
	}

	int bias = (int) (dwCurTime - (dwTime + lDelta));

	if (bias >= 0 && bias <= 50)
	{
		if (bInfiniteRetry)
		{
			BYTE bHeader = HEADER_GC_TIME_SYNC;
			Packet(&bHeader, sizeof(BYTE));
		}

		if (GetCharacter())
			sys_log(0, "Handshake: client_time %u server_time %u name: %s", m_dwClientTime, dwCurTime, GetCharacter()->GetName());
		else
			sys_log(0, "Handshake: client_time %u server_time %u", m_dwClientTime, dwCurTime, lDelta);

		m_dwClientTime = dwCurTime;
		m_bHandshaking = false;
		return true; 
	}

	long lNewDelta = (long) (dwCurTime - dwTime) / 2;

	if (lNewDelta < 0)
	{
		sys_log(0, "Handshake: lower than zero %d", lNewDelta);
		lNewDelta = (dwCurTime - m_dwHandshakeSentTime) / 2;
	}

	sys_log(1, "Handshake: ServerTime %u dwTime %u lDelta %d SentTime %u lNewDelta %d", dwCurTime, dwTime, lDelta, m_dwHandshakeSentTime, lNewDelta);

	if (!bInfiniteRetry)
		if (++m_iHandshakeRetry > HANDSHAKE_RETRY_LIMIT)
		{
			sys_err("handshake retry limit reached! (limit %d character %s)", 
					HANDSHAKE_RETRY_LIMIT, GetCharacter() ? GetCharacter()->GetName() : "!NO CHARACTER!");
			SetPhase(PHASE_CLOSE);
			return false;
		}

	SendHandshake(dwCurTime, lNewDelta);
	return false;
}

bool DESC::IsHandshaking()
{
	return m_bHandshaking;
}

DWORD DESC::GetClientTime()
{
	return m_dwClientTime;
}

#ifdef _IMPROVED_PACKET_ENCRYPTION_
void DESC::SendKeyAgreement()
{
	TPacketKeyAgreement packet;

	size_t data_length = TPacketKeyAgreement::MAX_DATA_LEN;
	size_t agreed_length = cipher_.Prepare(packet.data, &data_length);
	if (agreed_length == 0) {
		// Initialization failure
		SetPhase(PHASE_CLOSE);
		return;
	}
	assert(data_length <= TPacketKeyAgreement::MAX_DATA_LEN);

	packet.bHeader = HEADER_GC_KEY_AGREEMENT;
	packet.wAgreedLength = (WORD)agreed_length;
	packet.wDataLength = (WORD)data_length;

	Packet(&packet, sizeof(packet));
}

void DESC::SendKeyAgreementCompleted()
{
	TPacketKeyAgreementCompleted packet;

	packet.bHeader = HEADER_GC_KEY_AGREEMENT_COMPLETED;

	Packet(&packet, sizeof(packet));
}

bool DESC::FinishHandshake(size_t agreed_length, const void* buffer, size_t length)
{
	return cipher_.Activate(false, agreed_length, buffer, length);
}

bool DESC::IsCipherPrepared()
{
	return cipher_.IsKeyPrepared();
}
#endif // #ifdef _IMPROVED_PACKET_ENCRYPTION_

void DESC::SetRelay(const char * c_pszName)
{
	m_stRelayName = c_pszName;
}

void DESC::BindCharacter(LPCHARACTER ch)
{
	m_lpCharacter = ch;
}

void DESC::FlushOutput()
{
	if (m_sock == INVALID_SOCKET) {
		return;
	}

	if (buffer_size(m_lpOutputBuffer) <= 0)
		return;

	struct timeval sleep_tv, now_tv, start_tv;
	int event_triggered = false;

	gettimeofday(&start_tv, NULL);

	socket_block(m_sock);
	sys_log(0, "FLUSH START %d", buffer_size(m_lpOutputBuffer));

	while (buffer_size(m_lpOutputBuffer) > 0)
	{
		gettimeofday(&now_tv, NULL);

		int iSecondsPassed = now_tv.tv_sec - start_tv.tv_sec;

		if (iSecondsPassed > 10)
		{
			if (!event_triggered || iSecondsPassed > 20)
			{
				SetPhase(PHASE_CLOSE);
				break;
			}
		}

		sleep_tv.tv_sec = 0;
		sleep_tv.tv_usec = 10000;

		int num_events = fdwatch(m_lpFdw, &sleep_tv);

		if (num_events < 0)
		{
			sys_err("num_events < 0 : %d", num_events);
			break;
		}

		int event_idx;

		for (event_idx = 0; event_idx < num_events; ++event_idx)
		{
			LPDESC d2 = (LPDESC) fdwatch_get_client_data(m_lpFdw, event_idx);

			if (d2 != this)
				continue;

			switch (fdwatch_check_event(m_lpFdw, m_sock, event_idx))
			{
				case FDW_WRITE:
					event_triggered = true;

					if (ProcessOutput() < 0)
					{
						sys_err("Cannot flush output buffer");
						SetPhase(PHASE_CLOSE);
					}
					break;

				case FDW_EOF:
					SetPhase(PHASE_CLOSE);
					break;
			}
		}

		if (IsPhase(PHASE_CLOSE))
			break;
	}

	if (buffer_size(m_lpOutputBuffer) == 0)
		sys_log(0, "FLUSH SUCCESS");
	else
		sys_log(0, "FLUSH FAIL");

	usleep(250000);
}

EVENTFUNC(disconnect_event)
{
	DESC::desc_event_info* info = dynamic_cast<DESC::desc_event_info*>( event->info );

	if ( info == NULL )
	{
		sys_err( "disconnect_event> <Factor> Null pointer" );
		return 0;
	}

	LPDESC d = info->desc;

	d->m_pkDisconnectEvent = NULL;
	d->SetPhase(PHASE_CLOSE);
	return 0;
}

bool DESC::DelayedDisconnect(int iSec)
{
	if (m_pkDisconnectEvent != NULL) {
		return false;
	}

	desc_event_info* info = AllocEventInfo<desc_event_info>();
	info->desc = this;

	m_pkDisconnectEvent = event_create(disconnect_event, info, PASSES_PER_SEC(iSec));
	return true;
}

void DESC::DisconnectOfSameLogin()
{
	if (GetCharacter())
	{
		if (m_pkDisconnectEvent)
			return;

		GetCharacter()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("다른 컴퓨터에서 로그인 하여 접속을 종료 합니다."));
		DelayedDisconnect(5);
	}
	else
	{
		SetPhase(PHASE_CLOSE);
	}
}

void DESC::SetAdminMode()
{
	m_bAdminMode = true;
}

bool DESC::IsAdminMode()
{
	return m_bAdminMode;
}

BYTE DESC::GetSequence()
{
	return gc_abSequence[m_iCurrentSequence];
}

void DESC::SetNextSequence()
{
	if (++m_iCurrentSequence == SEQUENCE_MAX_NUM)
		m_iCurrentSequence = 0;
}

void DESC::SendLoginSuccessPacket()
{
	TAccountTable & rTable = GetAccountTable();

	TPacketGCLoginSuccess p;

	p.bHeader    = HEADER_GC_LOGIN_SUCCESS_NEWSLOT;

	p.handle     = GetHandle();
	p.random_key = DESC_MANAGER::instance().MakeRandomKey(GetHandle()); // FOR MARK
	thecore_memcpy(p.players, rTable.players, sizeof(rTable.players));

	for (int i = 0; i < PLAYER_PER_ACCOUNT; ++i)
	{   
		CGuild* g = CGuildManager::instance().GetLinkedGuild(rTable.players[i].dwID);

		if (g)
		{   
			p.guild_id[i] = g->GetID();
			strlcpy(p.guild_name[i], g->GetName(), sizeof(p.guild_name[i]));
		}   
		else
		{
			p.guild_id[i] = 0;
			p.guild_name[i][0] = '\0';
		}
	}

	Packet(&p, sizeof(TPacketGCLoginSuccess));
}

//void DESC::SendServerStatePacket(int nIndex)
//{
//	TPacketGCStateCheck rp;
//
//	int iTotal; 
//	int * paiEmpireUserCount;
//	int iLocal;
//
//	DESC_MANAGER::instance().GetUserCount(iTotal, &paiEmpireUserCount, iLocal);
//
//	rp.header	= 1; 
//	rp.key		= 0;
//	rp.index	= nIndex;
//
//	if (g_bNoMoreClient) rp.state = 0;
//	else rp.state = iTotal > g_iFullUserCount ? 3 : iTotal > g_iBusyUserCount ? 2 : 1;
//	
//	this->Packet(&rp, sizeof(rp));
//	//printf("STATE_CHECK PACKET PROCESSED.\n");
//}

void DESC::SetMatrixCardRowsAndColumns(unsigned long rows, unsigned long cols)
{
	m_dwMatrixRows = rows;
	m_dwMatrixCols = cols;
}

unsigned long DESC::GetMatrixRows()
{
	return m_dwMatrixRows;
}

unsigned long DESC::GetMatrixCols()
{
	return m_dwMatrixCols;
}

bool DESC::CheckMatrixTryCount()
{
	if (++m_bMatrixTryCount >= 3)
		return false;

	return true;
}

void DESC::SetLoginKey(DWORD dwKey)
{
	m_dwLoginKey = dwKey;
}

void DESC::SetLoginKey(CLoginKey * pkKey)
{
	m_pkLoginKey = pkKey;
	sys_log(0, "SetLoginKey %u", m_pkLoginKey->m_dwKey);
}

DWORD DESC::GetLoginKey()
{
	if (m_pkLoginKey)
		return m_pkLoginKey->m_dwKey;

	return m_dwLoginKey;
}

const BYTE* GetKey_20050304Myevan()
{   
	static bool bGenerated = false;
	static DWORD s_adwKey[1938]; 

	if (!bGenerated) 
	{
		bGenerated = true;
		DWORD seed = 1491971513; 

		for (UINT i = 0; i < BYTE(seed); ++i)
		{
			seed ^= 2148941891ul;
			seed += 3592385981ul;

			s_adwKey[i] = seed;
		}
	}

	return (const BYTE*)s_adwKey;
}

#ifndef _IMPROVED_PACKET_ENCRYPTION_
void DESC::SetSecurityKey(const DWORD * c_pdwKey)
{
	const BYTE * c_pszKey = (const BYTE *) "JyTxtHljHJlVJHorRM301vf@4fvj10-v";

	if (g_iUseLocale && !LC_IsKorea())
		c_pszKey = GetKey_20050304Myevan() + 37;

	thecore_memcpy(&m_adwDecryptionKey, c_pdwKey, 16);
	TEA_Encrypt(&m_adwEncryptionKey[0], &m_adwDecryptionKey[0], (const DWORD *) c_pszKey, 16);

	sys_log(0, "SetSecurityKey decrypt %u %u %u %u encrypt %u %u %u %u", 
			m_adwDecryptionKey[0], m_adwDecryptionKey[1], m_adwDecryptionKey[2], m_adwDecryptionKey[3],
			m_adwEncryptionKey[0], m_adwEncryptionKey[1], m_adwEncryptionKey[2], m_adwEncryptionKey[3]);
}
#endif // _IMPROVED_PACKET_ENCRYPTION_

void DESC::AssembleCRCMagicCube(BYTE bProcPiece, BYTE bFilePiece)
{
	static BYTE abXORTable[32] =
	{
		102,  30, 0, 0, 0, 0, 0, 0,
		188,  44, 0, 0, 0, 0, 0, 0,
		39, 201, 0, 0, 0, 0, 0, 0,
		43,   5, 0, 0, 0, 0, 0, 0,
	};

	bProcPiece = (bProcPiece ^ abXORTable[m_bCRCMagicCubeIdx]);
	bFilePiece = (bFilePiece ^ abXORTable[m_bCRCMagicCubeIdx+1]);

	m_dwProcCRC |= bProcPiece << m_bCRCMagicCubeIdx;
	m_dwFileCRC |= bFilePiece << m_bCRCMagicCubeIdx;

	m_bCRCMagicCubeIdx += 8;

	if (!(m_bCRCMagicCubeIdx & 31))
	{
		m_dwProcCRC = 0;
		m_dwFileCRC = 0;
		m_bCRCMagicCubeIdx = 0;
	}
}

void DESC::SetBillingExpireSecond(DWORD dwSec)
{
	m_dwBillingExpireSecond = dwSec;
}

DWORD DESC::GetBillingExpireSecond()
{
	return m_dwBillingExpireSecond;
}

void DESC::push_seq(BYTE hdr, BYTE seq)
{
	if (m_seq_vector.size()>=20)
	{
		m_seq_vector.erase(m_seq_vector.begin());
	}

	seq_t info = { hdr, seq };
	m_seq_vector.push_back(info);
}

BYTE DESC::GetEmpire()
{
	return m_accountTable.bEmpire;
}

void DESC::ChatPacket(BYTE type, const char * format, ...)
{
	char chatbuf[CHAT_MAX_LEN + 1];
	va_list args;

	va_start(args, format);
	int len = vsnprintf(chatbuf, sizeof(chatbuf), format, args);
	va_end(args);

	struct packet_chat pack_chat;

	pack_chat.header    = HEADER_GC_CHAT;
	pack_chat.size      = sizeof(struct packet_chat) + len;
	pack_chat.type      = type;
	pack_chat.id        = 0;
	pack_chat.bEmpire   = GetEmpire();

	TEMP_BUFFER buf;
	buf.write(&pack_chat, sizeof(struct packet_chat));
	buf.write(chatbuf, len);

	Packet(buf.read_peek(), buf.size());
}

