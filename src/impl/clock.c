#include "clock.h"

#if defined(__APPLE__)
#include <mach/mach_time.h>
#include <time.h>

static uint64_t sg_clock_zero;
static struct mach_timebase_info sg_clock_info;

void
sg_clock_init(void)
{
    mach_timebase_info(&sg_clock_info);
    sg_clock_zero = mach_absolute_time();
}

unsigned
sg_clock_get(void)
{
    uint64_t tv = mach_absolute_time();
    return (tv - sg_clock_zero) * sg_clock_info.numer
        / ((uint64_t) sg_clock_info.denom * 1000000);
}

#elif defined(_WIN32)
#include <windows.h>

static DWORD sg_clock_zero;

void
sg_clock_init(void)
{
    sg_clock_zero = GetTickCount();
}

unsigned
sg_clock_get(void)
{
    DWORD tv = GetTickCount();
    return tv - sg_clock_zero;
}

#else

#include <time.h>
#include <unistd.h>
#if _POSIX_TIMERS && defined(_POSIX_MONOTONIC_CLOCK)

static struct timespec sg_clock_zero;

void
sg_clock_init(void)
{
    clock_gettime(CLOCK_MONOTONIC, &sg_clock_zero);
}

unsigned
sg_clock_get(void)
{
    struct timespec tv;
    clock_gettime(CLOCK_MONOTONIC, &tv);
    return (tv.tv_sec - sg_clock_zero.tv_sec) * 1000
        + (tv.tv_nsec - sg_clock_zero.tv_nsec) / 1000000;
}

#else
#include <sys/time.h>

static struct timeval sg_clock_zero;

void
sg_clock_init(void)
{
    gettimeofday(&sg_clock_zero, NULL);
}

unsigned
sg_clock_get(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec - sg_clock_zero.tv_sec) * 1000
        + (tv.tv_usec - sg_clock_zero.tv_usec) / 1000;
}

#endif

#endif
