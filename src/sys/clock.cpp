#if defined(__APPLE__)
#include <mach/mach_time.h>
#include <time.h>

static uint64_t ClockZero;
static struct mach_timebase_info ClockInfo;

void initTime(void)
{
    mach_timebase_info(&ClockInfo);
    ClockZero = mach_absolute_time();
}

unsigned getTime(void)
{
    uint64_t tv = mach_absolute_time();
    return (tv - ClockZero) * ClockInfo.numer
        / ((uint64_t) ClockInfo.denom * 1000000);
}

#elif defined(_WIN32)
#include <windows.h>

static DWORD ClockZero;

void initTime(void)
{
    ClockZero = GetTickCount();
}

unsigned getTime(void)
{
    DWORD tv = GetTickCount();
    return tv - ClockZero;
}

#else

#include <time.h>
#include <unistd.h>
#if _POSIX_TIMERS && defined(_POSIX_MONOTONIC_CLOCK)

static struct timespec ClockZero;

void initTime()
{
    clock_gettime(CLOCK_MONOTONIC, &ClockZero);
}

unsigned getTime()
{
    struct timespec tv;
    clock_gettime(CLOCK_MONOTONIC, &tv);
    return (tv.tv_sec - ClockZero.tv_sec) * 1000
        + (tv.tv_nsec - ClockZero.tv_nsec) / 1000000;
}

#else
#include <sys/time.h>

static struct timeval ClockZero;

void initTime()
{
    gettimeofday(&ClockZero, NULL);
}

unsigned getTime()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec - ClockZero.tv_sec) * 1000
        + (tv.tv_usec - ClockZero.tv_usec) / 1000;
}

#endif

#endif
