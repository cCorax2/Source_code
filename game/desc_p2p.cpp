#include "stdafx.h"
#include "desc_p2p.h"
#include "protocol.h"
#include "p2p.h"

DESC_P2P::~DESC_P2P()
{
}

void DESC_P2P::Destroy()
{
	if (m_sock == INVALID_SOCKET) {
		return;
	}

	P2P_MANAGER::instance().UnregisterAcceptor(this);

	fdwatch_del_fd(m_lpFdw, m_sock);

	sys_log(0, "SYSTEM: closing p2p socket. DESC #%d", m_sock);

	socket_close(m_sock);
	m_sock = INVALID_SOCKET;

	// Chain up to base class Destroy()
	DESC::Destroy();
}

bool DESC_P2P::Setup(LPFDWATCH fdw, socket_t fd, const char * host, WORD wPort)
{
	m_lpFdw = fdw;
	m_stHost = host;
	m_wPort = wPort;
	m_sock = fd;

	if (!(m_lpOutputBuffer = buffer_new(1024 * 1024)))
		return false;

	if (!(m_lpInputBuffer = buffer_new(1024 * 1024)))
		return false;

	fdwatch_add_fd(m_lpFdw, m_sock, this, FDW_READ, false);

	m_iMinInputBufferLen = 1024 * 1024;

	SetPhase(PHASE_P2P);

	sys_log(0, "SYSTEM: new p2p connection from [%s] fd: %d", host, m_sock);
	return true;
}

void DESC_P2P::SetPhase(int iPhase)
{
	static CInputP2P s_inputP2P;

	switch (iPhase)
	{
		case PHASE_P2P:
			sys_log(1, "PHASE_P2P");

			if (m_lpInputBuffer)
				buffer_reset(m_lpInputBuffer);

			if (m_lpOutputBuffer)
				buffer_reset(m_lpOutputBuffer);

			m_pInputProcessor = &s_inputP2P;
			break;

		case PHASE_CLOSE:
			m_pInputProcessor = NULL;
			break;

		default:
			sys_err("DESC_P2P::SetPhase : Unknown phase");
			break;
	}

	m_iPhase = iPhase;
}

