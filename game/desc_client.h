#ifndef __CLIENT_H__
#define __CLIENT_H__

#include "desc.h"

class CLIENT_DESC : public DESC
{
	public:
		CLIENT_DESC();
		virtual ~CLIENT_DESC();

		virtual BYTE	GetType() { return DESC_TYPE_CONNECTOR; }
		virtual void	Destroy();
		virtual void	SetPhase(int phase);

		bool 		Connect(int iPhaseWhenSucceed = 0);
		void		Setup(LPFDWATCH _fdw, const char * _host, WORD _port);

		void		SetRetryWhenClosed(bool);

		void		DBPacketHeader(BYTE bHeader, DWORD dwHandle, DWORD dwSize);
		void		DBPacket(BYTE bHeader, DWORD dwHandle, const void * c_pvData, DWORD dwSize);
		void		Packet(const void * c_pvData, int iSize);
		bool		IsRetryWhenClosed();

		void		Update(DWORD t);
		void		UpdateChannelStatus(DWORD t, bool fForce);

		// Non-destructive close for reuse
		void Reset();

	private:
		void InitializeBuffers();

	protected:
		int			m_iPhaseWhenSucceed;
		bool		m_bRetryWhenClosed;
		time_t		m_LastTryToConnectTime;
		time_t		m_tLastChannelStatusUpdateTime;

		CInputDB 	m_inputDB;
		CInputP2P 	m_inputP2P;
		CInputTeen	m_inputTeen;
};


extern LPCLIENT_DESC db_clientdesc;
extern LPCLIENT_DESC g_pkAuthMasterDesc;
extern LPCLIENT_DESC g_NetmarbleDBDesc;
extern LPCLIENT_DESC g_TeenDesc;
extern LPCLIENT_DESC g_PasspodDesc;

#endif
