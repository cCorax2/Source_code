#ifndef __INC_LIBTHECORE_MEMCPY_H__
#define __INC_LIBTHECORE_MEMCPY_H__

#ifdef __cplusplus
extern "C"
{
#endif
#ifdef __LIBTHECORE__
    void thecore_find_best_memcpy();
#endif

#ifndef __WIN32__
    extern void *(*thecore_memcpy) (void * to, const void * from, size_t len);
#else
#include <cstring>
#define thecore_memcpy memcpy
#endif

#ifdef __cplusplus
};
#endif

#endif
