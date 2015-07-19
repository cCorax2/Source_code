/*********************************************************************
 * date        : 2006.09.07
 * file        : dev_log.h
 * author      : mhh
 * description : 개발자용 로그함수 테스트 서버에서만 기록된다.
 *				example)
 *				dev_log(LOG_DEB0, "My Name is %s", name);
 */

#ifndef _dev_log_h_
#define _dev_log_h_


// -----------------------------------------------
// define log level
// -----------------------------------------------
#define L_WARN  (1<<1)
#define L_ERR   (1<<2)
#define L_CRIT  (1<<3)
#define L_INFO  (1<<4)

#define L_MIN   (1<<5)
#define L_MAX   (1<<6)

#define L_LIB0  (1<<7)
#define L_LIB1  (1<<8)
#define L_LIB2  (1<<9)
#define L_LIB3  (1<<10)

#define L_DEB0  (1<<11)
#define L_DEB1  (1<<12)
#define L_DEB2  (1<<13)
#define L_DEB3  (1<<14)


#define L_USR0  (1<<15)
#define L_USR1  (1<<16)
#define L_USR2  (1<<17)
#define L_USR3  (1<<18)


// -----------------------------------------------
// define log level
// -----------------------------------------------
#define LOG_WARN        __FILE__,__LINE__,__FUNCTION__,L_WARN
#define LOG_ERR         __FILE__,__LINE__,__FUNCTION__,L_ERR
#define LOG_CRIT        __FILE__,__LINE__,__FUNCTION__,L_CRIT
#define LOG_INFO        __FILE__,__LINE__,__FUNCTION__,L_INFO

#define LOG_MIN         __FILE__,__LINE__,__FUNCTION__,L_MIN
#define LOG_MAX         __FILE__,__LINE__,__FUNCTION__,L_MAX

#define LOG_LIB0        __FILE__,__LINE__,__FUNCTION__,L_LIB0
#define LOG_LIB1        __FILE__,__LINE__,__FUNCTION__,L_LIB1
#define LOG_LIB2        __FILE__,__LINE__,__FUNCTION__,L_LIB2
#define LOG_LIB3        __FILE__,__LINE__,__FUNCTION__,L_LIB3

#define LOG_DEB0        __FILE__,__LINE__,__FUNCTION__,L_DEB0
#define LOG_DEB1        __FILE__,__LINE__,__FUNCTION__,L_DEB1
#define LOG_DEB2        __FILE__,__LINE__,__FUNCTION__,L_DEB2
#define LOG_DEB3        __FILE__,__LINE__,__FUNCTION__,L_DEB3


#define LOG_USR0        __FILE__,__LINE__,__FUNCTION__,L_USR0
#define LOG_USR1        __FILE__,__LINE__,__FUNCTION__,L_USR1
#define LOG_USR2        __FILE__,__LINE__,__FUNCTION__,L_USR2
#define LOG_USR3        __FILE__,__LINE__,__FUNCTION__,L_USR3




void dev_log(const char *file, int line, const char *function, int level, const char *fmt, ...);
void dev_log_add_level(int level);
void dev_log_del_level(int level);
void dev_log_set_level(int mask);


#endif	/* _dev_log_h_ */

