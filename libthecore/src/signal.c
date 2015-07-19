/*
 *    Filename: signal.c
 * Description: 시그널 관련 함수.
 *
 *      Author: 비엽 aka. Cronan
 */
#define __LIBTHECORE__
#include "stdafx.h"

#ifdef __WIN32__
void signal_setup() {}
void signal_timer_disable() {}
void signal_timer_enable(int timeout_seconds) {}
#elif __FreeBSD__
#define RETSIGTYPE void

RETSIGTYPE reap(int sig)
{
    while (waitpid(-1, NULL, WNOHANG) > 0);
    signal(SIGCHLD, reap);
}


RETSIGTYPE checkpointing(int sig)
{
    if (!tics)
    {
	sys_err("CHECKPOINT shutdown: tics did not updated.");
	abort();
    }
    else
	tics = 0;
}


RETSIGTYPE hupsig(int sig)
{
    shutdowned = TRUE;
    sys_err("SIGHUP, SIGINT, SIGTERM signal has been received. shutting down.");
}

RETSIGTYPE usrsig(int sig)
{
    core_dump();
}

void signal_timer_disable(void)
{
    struct itimerval itime;
    struct timeval interval;

    interval.tv_sec	= 0;
    interval.tv_usec	= 0;

    itime.it_interval = interval;
    itime.it_value = interval;

    setitimer(ITIMER_VIRTUAL, &itime, NULL);
}

void signal_timer_enable(int sec)
{
    struct itimerval itime;
    struct timeval interval;

    interval.tv_sec	= sec;
    interval.tv_usec	= 0;

    itime.it_interval = interval;
    itime.it_value = interval;

    setitimer(ITIMER_VIRTUAL, &itime, NULL);
}

void signal_setup(void)
{
    signal_timer_enable(30);

    signal(SIGVTALRM, checkpointing);

    /* just to be on the safe side: */
    signal(SIGHUP, hupsig);
    signal(SIGCHLD, reap);
    signal(SIGINT, hupsig);
    signal(SIGTERM, hupsig);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGALRM, SIG_IGN);
    signal(SIGUSR1, usrsig);
}

#endif
