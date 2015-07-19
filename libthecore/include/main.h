#ifndef __INC_LIBTHECORE_MAIN_H__
#define __INC_LIBTHECORE_MAIN_H__

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#ifdef __LIBTHECORE__
    extern volatile int	tics;
    extern volatile int shutdowned;
#endif
#include "heart.h"

    extern LPHEART	thecore_heart;

    enum ENUM_PROFILER
    {
	PF_IDLE,
	PF_HEARTBEAT,
	NUM_PF
    };

    extern unsigned int		thecore_profiler[NUM_PF];

    extern int			thecore_init(int fps, HEARTFUNC heartbeat_func);
    extern int			thecore_idle(void);
    extern void			thecore_shutdown(void);
    extern void			thecore_destroy(void);
    extern int			thecore_pulse(void);
    extern float		thecore_time(void);
    extern float		thecore_pulse_per_second(void);
	extern int			thecore_is_shutdowned(void);

	extern void			thecore_tick(void); // tics ¡ı∞°

#ifdef __cplusplus
}
#endif
#endif
