// vim: ts=8 sw=4
#ifndef __INC_PEERBASE_H__
#define __INC_PEERBASE_H__

#include "NetBase.h"

class CPeerBase : public CNetBase
{
    public:
	enum
	{ 
	    MAX_HOST_LENGTH		= 30,
	    MAX_INPUT_LEN		= 1024 * 1024 * 2,
	    DEFAULT_PACKET_BUFFER_SIZE	= 1024 * 1024 * 2
	};

    protected:
	virtual void	OnAccept() = 0;
	virtual void	OnConnect() = 0;
	virtual void	OnClose() = 0;

    public:
	bool		Accept(socket_t accept_fd);
	bool		Connect(const char* host, WORD port);
	void		Close();

    public:
	CPeerBase();
	virtual ~CPeerBase();

	void		Disconnect();
	void		Destroy();

	socket_t	GetFd() { return m_fd; }

	void		EncodeBYTE(BYTE b);
	void		EncodeWORD(WORD w);
	void		EncodeDWORD(DWORD dw);
	void		Encode(const void* data, DWORD size);
	int		Send();

	int		Recv();
	void		RecvEnd(int proceed_bytes);
	int		GetRecvLength();
	const void *	GetRecvBuffer();

	int		GetSendLength();

	const char *	GetHost() { return m_host; }

    protected:
	char		m_host[MAX_HOST_LENGTH + 1];
	socket_t	m_fd;

    private:
	int		m_BytesRemain;
	LPBUFFER	m_outBuffer;
	LPBUFFER	m_inBuffer;
};

#endif
