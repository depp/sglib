#include "clock.hpp"
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
