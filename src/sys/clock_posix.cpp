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
