/*
 *    Filename: socket.c
 * Description: 소켓 관련 소스.
 *
 *      Author: 비엽 aka. Cronan
 */
#define __LIBTHECORE__
#include "stdafx.h"

/* Forwards */
void socket_lingeron(socket_t s);
void socket_lingeroff(socket_t s);
void socket_timeout(socket_t s, long sec, long usec);
void socket_reuse(socket_t s);
void socket_keepalive(socket_t s);

int socket_udp_read(socket_t desc, char * read_point, size_t space_left, struct sockaddr * from, socklen_t * fromlen)
{
    /*
       ssize_t recvfrom(int s, void * buf, size_t len, int flags, struct sockaddr * from, socklen_t * fromlen);
     */
    ssize_t ret;
    ret = recvfrom(desc, read_point, space_left, 0, from, fromlen);
    return (ret);
}

int socket_read(socket_t desc, char* read_point, size_t space_left)
{
    int	ret;

    ret = recv(desc, read_point, space_left, 0);

    if (ret > 0)
	return ret;

    if (ret == 0)	// 정상적으로 접속 끊김
	return -1;

#ifdef EINTR            /* Interrupted system call - various platforms */
    if (errno == EINTR)
	return (0);
#endif

#ifdef EAGAIN           /* POSIX */
    if (errno == EAGAIN)
	return (0);
#endif

#ifdef EWOULDBLOCK      /* BSD */
    if (errno == EWOULDBLOCK)
	return (0);
#endif /* EWOULDBLOCK */

#ifdef EDEADLK          /* Macintosh */
    if (errno == EDEADLK)
	return (0);
#endif

#ifdef __WIN32__
	int wsa_error = WSAGetLastError();
	if (wsa_error == WSAEWOULDBLOCK) {
		return 0;
	}
	sys_log(0, "socket_read: WSAGetLastError returned %d", wsa_error);
#endif

    sys_err("about to lose connection");
    return -1;
}


int socket_write_tcp(socket_t desc, const char *txt, int length)
{
    int bytes_written = send(desc, txt, length, 0);

    // 성공
    if (bytes_written > 0)
	return (bytes_written);

    if (bytes_written == 0)
	return -1;

#ifdef EAGAIN           /* POSIX */
    if (errno == EAGAIN)
	return 0;
#endif

#ifdef EWOULDBLOCK      /* BSD */
    if (errno == EWOULDBLOCK)
	return 0;
#endif

#ifdef EDEADLK          /* Macintosh */
    if (errno == EDEADLK)
	return 0;
#endif

#ifdef __WIN32__
	int wsa_error = WSAGetLastError();
	if (wsa_error == WSAEWOULDBLOCK) {
		return 0;
	}
	sys_log(0, "socket_write_tcp: WSAGetLastError returned %d", wsa_error);
#endif

    /* Looks like the error was fatal.  Too bad. */
    return -1;
}


int socket_write(socket_t desc, const char *data, size_t length)
{
    size_t	total;
    int		bytes_written;

    total = length;

    do
    {
	if ((bytes_written = socket_write_tcp(desc, data, total)) < 0)
	{
#ifdef EWOULDBLOCK
	    if (errno == EWOULDBLOCK)
		errno = EAGAIN;
#endif
	    if (errno == EAGAIN)
		sys_err("socket write would block, about to close!");
	    else
		sys_err("write to desc error");   // '보통' 상대편으로 부터 접속이 끊긴 것이다.

	    return -1;
	}
	else
	{
	    data        += bytes_written;
	    total       -= bytes_written;
	}
    }
    while (total > 0);

    return 0;
}

int socket_bind(const char * ip, int port, int protocol)
{
    int                 s;
#ifdef __WIN32
    SOCKADDR_IN			sa;
#else
    struct sockaddr_in  sa;
#endif

    if ((s = socket(AF_INET, protocol, 0)) < 0) 
    {
	sys_err("socket: %s", strerror(errno));
	return -1;
    }

    socket_reuse(s);
#ifndef __WIN32__
    socket_lingeroff(s);
#else
	// Winsock2: SO_DONTLINGER, SO_KEEPALIVE, SO_LINGER, and SO_OOBINLINE are 
	// not supported on sockets of type SOCK_DGRAM
	if (protocol == SOCK_STREAM) {
		socket_lingeroff(s);
	}
#endif

    memset(&sa, 0, sizeof(sa));
    sa.sin_family	= AF_INET;
//윈도우 서버는 개발용으로만 쓰기 때문에 BIND ip를 INADDR_ANY로 고정
//(테스트의 편의성을 위해)
#ifndef __WIN32__
    sa.sin_addr.s_addr	= inet_addr(ip);
#else
	sa.sin_addr.s_addr	= INADDR_ANY;
#endif 
    sa.sin_port		= htons((unsigned short) port);

    if (bind(s, (struct sockaddr *) &sa, sizeof(sa)) < 0)
    {
	sys_err("bind: %s", strerror(errno));
	return -1;
    }

    socket_nonblock(s);

    if (protocol == SOCK_STREAM)
    {
	sys_log(0, "SYSTEM: BINDING TCP PORT ON [%d] (fd %d)", port, s);
	listen(s, SOMAXCONN);
    }
    else
	sys_log(0, "SYSTEM: BINDING UDP PORT ON [%d] (fd %d)", port, s);

    return s;
}

int socket_tcp_bind(const char * ip, int port)
{
    return socket_bind(ip, port, SOCK_STREAM);
}

int socket_udp_bind(const char * ip, int port)
{
    return socket_bind(ip, port, SOCK_DGRAM);
}

void socket_close(socket_t s)
{
#ifdef __WIN32__
    closesocket(s);
#else
    close(s);
#endif
}

socket_t socket_accept(socket_t s, struct sockaddr_in *peer)
{
    socket_t desc;
    socklen_t i;

    i = sizeof(*peer);

    if ((desc = accept(s, (struct sockaddr *) peer, &i)) == -1)
    {
	sys_err("accept: %s (fd %d)", strerror(errno), s);
	return -1;
    }

    if (desc >= 65500)
    {
	sys_err("SOCKET FD 65500 LIMIT! %d", desc);
	socket_close(s);
	return -1;
    }

    socket_nonblock(desc);
    socket_lingeroff(desc);
    return (desc);
}

socket_t socket_connect(const char* host, WORD port)
{
    socket_t            s = 0;
    struct sockaddr_in  server_addr;
    int                 rslt;

    /* 소켓주소 구조체 초기화 */
    memset(&server_addr, 0, sizeof(server_addr));

    if (isdigit(*host))
	server_addr.sin_addr.s_addr = inet_addr(host);
    else
    {
	struct hostent *hp;

	if ((hp = gethostbyname(host)) == NULL)
	{
	    sys_err("socket_connect(): can not connect to %s:%d", host, port);
	    return -1;
	}

	thecore_memcpy((char* ) &server_addr.sin_addr, hp->h_addr, sizeof(server_addr.sin_addr));
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
	perror("socket");
	return -1;
    }

    socket_keepalive(s);
    socket_sndbuf(s, 233016);
    socket_rcvbuf(s, 233016);
    socket_timeout(s, 10, 0);
    socket_lingeron(s);

    /*  연결요청 */
    if ((rslt = connect(s, (struct sockaddr *) &server_addr, sizeof(server_addr))) < 0)
    {
	socket_close(s);

#ifdef __WIN32__
	switch (WSAGetLastError())
#else
	    switch (rslt)
#endif
	    {
#ifdef __WIN32__
		case WSAETIMEDOUT:
#else
		case EINTR:
#endif
		    sys_err("HOST %s:%d connection timeout.", host, port);
		    break;
#ifdef __WIN32__
		case WSAECONNREFUSED:
#else
		case ECONNREFUSED:
#endif
		    sys_err("HOST %s:%d port is not opened. connection refused.", host, port);
		    break;
#ifdef __WIN32__
		case WSAENETUNREACH:
#else
		case ENETUNREACH:
#endif
		    sys_err("HOST %s:%d is not reachable from this host.", host, port);
		    break;

		default:
		    sys_err("HOST %s:%d, could not connect.", host, port);
		    break;
	    }

	perror("connect");
	return (-1);
    }

    return (s);
}

#ifndef __WIN32__

#ifndef O_NONBLOCK
#define O_NONBLOCK O_NDELAY
#endif

void socket_nonblock(socket_t s)
{
    int flags;

    flags = fcntl(s, F_GETFL, 0);
    flags |= O_NONBLOCK;

    if (fcntl(s, F_SETFL, flags) < 0) 
    {
	sys_err("fcntl: nonblock: %s", strerror(errno));
	return;
    }
}

void socket_block(socket_t s)
{
    int flags;

    flags = fcntl(s, F_GETFL, 0);
    flags &= ~O_NONBLOCK;

    if (fcntl(s, F_SETFL, flags) < 0)
    {
	sys_err("fcntl: nonblock: %s", strerror(errno));
	return;
    }
}
#else
void socket_nonblock(socket_t s)
{
    unsigned long val = 1;
    ioctlsocket(s, FIONBIO, &val);
}

void socket_block(socket_t s)
{
    unsigned long val = 0;
    ioctlsocket(s, FIONBIO, &val);
}
#endif

void socket_dontroute(socket_t s)
{
    int set;

    if (setsockopt(s, SOL_SOCKET, SO_DONTROUTE, (const char *) &set, sizeof(int)) < 0)
    {
	sys_err("setsockopt: dontroute: %s", strerror(errno));
	socket_close(s);
	return;
    }
}

void socket_lingeroff(socket_t s)
{
#ifdef __WIN32__
    int linger;
    linger = 0;
#else
    struct linger linger;

    linger.l_onoff	= 0;
    linger.l_linger	= 0;
#endif
    if (setsockopt(s, SOL_SOCKET, SO_LINGER, (const char*) &linger, sizeof(linger)) < 0)
    {
	sys_err("setsockopt: linger: %s", strerror(errno));
	socket_close(s);
	return;
    }
}

void socket_lingeron(socket_t s)
{
#ifdef __WIN32__
    int linger;
    linger = 0;
#else
    struct linger linger;

    linger.l_onoff	= 1;
    linger.l_linger	= 0;
#endif
    if (setsockopt(s, SOL_SOCKET, SO_LINGER, (const char*) &linger, sizeof(linger)) < 0)
    {
	sys_err("setsockopt: linger: %s", strerror(errno));
	socket_close(s);
	return;
    }
}

void socket_rcvbuf(socket_t s, unsigned int opt)
{
    socklen_t optlen;

    optlen = sizeof(opt);

    if (setsockopt(s, SOL_SOCKET, SO_RCVBUF, (const char*) &opt, optlen) < 0)
    {
	sys_err("setsockopt: rcvbuf: %s", strerror(errno));
	socket_close(s);
	return;
    }

    opt         = 0;
    optlen      = sizeof(opt);

    if (getsockopt(s, SOL_SOCKET, SO_RCVBUF, (char*) &opt, &optlen) < 0)
    {
	sys_err("getsockopt: rcvbuf: %s", strerror(errno));
	socket_close(s);
	return;
    }

    sys_log(1, "SYSTEM: %d: receive buffer changed to %d", s, opt);
}

void socket_sndbuf(socket_t s, unsigned int opt)
{
    socklen_t optlen;

    optlen = sizeof(opt);

    if (setsockopt(s, SOL_SOCKET, SO_SNDBUF, (const char*) &opt, optlen) < 0)
    {
	sys_err("setsockopt: sndbuf: %s", strerror(errno));
	return;
    }

    opt         = 0;
    optlen      = sizeof(opt);

    if (getsockopt(s, SOL_SOCKET, SO_SNDBUF, (char*) &opt, &optlen) < 0)
    {
	sys_err("getsockopt: sndbuf: %s", strerror(errno));
	return;
    }

    sys_log(1, "SYSTEM: %d: send buffer changed to %d", s, opt);
}

// sec : seconds, usec : microseconds
void socket_timeout(socket_t s, long sec, long usec)
{
#ifndef __WIN32__
    struct timeval      rcvopt;
    struct timeval      sndopt;
    socklen_t		optlen = sizeof(rcvopt);

    rcvopt.tv_sec = sndopt.tv_sec = sec;
    rcvopt.tv_usec = sndopt.tv_usec = usec;
#else
    socklen_t		rcvopt, sndopt;
    socklen_t		optlen = sizeof(rcvopt);
    sndopt = rcvopt = (sec * 1000) + (usec / 1000);
#endif
    if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char*) &rcvopt, optlen) < 0)
    {
	sys_err("setsockopt: timeout: %s", strerror(errno));
	socket_close(s);
	return;
    }

    if (getsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char*) &rcvopt, &optlen) < 0)
    {
	sys_err("getsockopt: timeout: %s", strerror(errno));
	socket_close(s);
	return;
    }

    optlen = sizeof(sndopt);

    if (setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (const char*) &sndopt, optlen) < 0)
    {
	sys_err("setsockopt: timeout: %s", strerror(errno));
	socket_close(s);
	return;
    }

    if (getsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (char*) &sndopt, &optlen) < 0)
    {
	sys_err("getsockopt: timeout: %s", strerror(errno));
	socket_close(s);
	return;
    }

#ifndef __WIN32__
    sys_log(1, "SYSTEM: %d: TIMEOUT RCV: %d.%d, SND: %d.%d", s, rcvopt.tv_sec, rcvopt.tv_usec, sndopt.tv_sec, sndopt.tv_usec);
#endif
}

void socket_reuse(socket_t s)
{
    int opt = 1;

    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char*) &opt, sizeof(opt)) < 0)
    {
	sys_err("setsockopt: reuse: %s", strerror(errno));
	socket_close(s);
	return;
    }
}

void socket_keepalive(socket_t s)
{
    int opt = 1;

    if (setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, (const char*) &opt, sizeof(opt)) < 0)
    {
	perror("setsockopt: keepalive");
	socket_close(s);
	return;
    }
}
