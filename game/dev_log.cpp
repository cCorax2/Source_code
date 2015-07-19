/*********************************************************************
 * date        : 2006.09.07
 * file        : dev_log.cpp
 * author      : mhh
 * description : 
 */

#define _dev_log_cpp_

#include "stdafx.h"
#include "dev_log.h"

#ifndef IS_SET
#	define IS_SET(flag,bit)		((flag) & (bit))
#endif	// IS_SET

#ifndef SET_BIT
#	define SET_BIT(var,bit)		((var) |= (bit))
#endif	// SET_BIT

#ifndef REMOVE_BIT
#	define REMOVE_BIT(var,bit)		((var) &= ~(bit))
#endif	// REMOVE_BIT

#ifndef TOGGLE_BIT
#	define TOGGLE_BIT(var,bit)		((var) = (var) ^ (bit))
#endif	// TOGGLE_BIT

extern int test_server;
static int	s_log_mask = 0xffffffff;

void dev_log(const char *file, int line, const char *function, int level, const char *fmt, ...)
{
	// 테스트 서버에서만 남기며, 마스크가 꺼져있으면 남기지 않는다.
	if (!test_server || !IS_SET(s_log_mask, level))
		return;

	static char	buf[1024*1024];	// 1M
	int			fd;
    char        strtime[64+1];
    const char        *strlevel;
    struct      timeval tv;
    struct      tm      ltm;
    int         mon, day, hour, min, sec, usec;
    int         nlen;
    va_list     args;

	// ---------------------------------------
	// open file
	// ---------------------------------------
#ifndef __WIN32__
	fd = ::open("DEV_LOG.log", O_WRONLY|O_APPEND|O_CREAT, 0666);
#else
	fd = ::_open("DEV_LOG.log", _O_WRONLY|_O_APPEND|_O_CREAT, 0666);
#endif

	if (fd < 0)
		return;

    // ---------------------------------------
    // set time string
    // ---------------------------------------
    gettimeofday (&tv, NULL);

    localtime_r((time_t*) &tv.tv_sec, &ltm);

    mon         = ltm.tm_mon + 1;
    day         = ltm.tm_mday;
    hour        = ltm.tm_hour;
    min         = ltm.tm_min;
    sec         = ltm.tm_sec;
    usec        = tv.tv_usec;

	nlen = snprintf(strtime, sizeof(strtime), "%02d%02d %02d:%02d.%02d.%06d", mon, day, hour, min, sec, usec);

	if (nlen < 0 || nlen >= (int) sizeof(strtime))
		nlen = sizeof(strtime) - 1;

	strtime[nlen - 2] = '\0';

    // ---------------------------------------
    // get level string
    // ---------------------------------------
#define GET_LEVEL_STRING(level) case L_##level : strlevel = #level; break
    switch ( level ) {
        GET_LEVEL_STRING ( WARN );
        GET_LEVEL_STRING ( ERR );
        GET_LEVEL_STRING ( CRIT );
        GET_LEVEL_STRING ( INFO );

        GET_LEVEL_STRING ( MIN );
        GET_LEVEL_STRING ( MAX );

        GET_LEVEL_STRING ( LIB0 );
        GET_LEVEL_STRING ( LIB1 );
        GET_LEVEL_STRING ( LIB2 );
        GET_LEVEL_STRING ( LIB3 );

        GET_LEVEL_STRING ( DEB0 );
        GET_LEVEL_STRING ( DEB1 );
        GET_LEVEL_STRING ( DEB2 );
        GET_LEVEL_STRING ( DEB3 );

        GET_LEVEL_STRING ( USR0 );
        GET_LEVEL_STRING ( USR1 );
        GET_LEVEL_STRING ( USR2 );
        GET_LEVEL_STRING ( USR3 );

        default : strlevel = "UNKNOWN"; break;
    }
#undef GET_LEVEL_STRING

	nlen = snprintf(buf, sizeof(buf), "%s %-4s (%-15s,%4d,%-24s) ",
			strtime, strlevel, file, line, function);

	if (nlen < 0 || nlen >= (int) sizeof(buf))
		return;

	// ---------------------------------------
	// write_log
	// ---------------------------------------
	va_start(args, fmt);
	int vlen = vsnprintf(buf + nlen, sizeof(buf) - (nlen + 2), fmt, args);
	va_end(args);

	if (vlen < 0 || vlen >= (int) sizeof(buf) - (nlen + 2))
		nlen += (sizeof(buf) - (nlen + 2)) - 1;
	else
		nlen += vlen;

	buf[nlen++] = '\n';
	buf[nlen] = 0;

	::write(fd, buf, nlen);
	::close(fd);
}

void dev_log_add_level(int level)
{
	SET_BIT(s_log_mask, level);
}

void dev_log_del_level(int level)
{
	REMOVE_BIT(s_log_mask, level);
}

void dev_log_set_level(int mask)
{
	s_log_mask = mask;
}

