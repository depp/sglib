/* Copyright 2012 Dietrich Epp <depp@zdome.net> */
#include "clock_impl.h"
#include "sg/clock.h"
#include <stdio.h>

#if defined(_MSC_VER)
#define snprintf _snprintf
#endif

#if defined(SG_CLOCK_APPLE)

uint64_t sg_clock_zero;
struct mach_timebase_info sg_clock_info;

unsigned
sg_clock_convert(uint64_t mach_time)
{
    return (mach_time - sg_clock_zero) * sg_clock_info.numer /
        ((uint64_t) sg_clock_info.denom * 1000000);
}

void
sg_clock_init(void)
{
    mach_timebase_info(&sg_clock_info);
    sg_clock_zero = mach_absolute_time();
}

unsigned
sg_clock_get(void)
{
    return sg_clock_convert(mach_absolute_time());
}

#elif defined(SG_CLOCK_WINDOWS)

DWORD sg_clock_zero;

unsigned
sg_clock_convert(DWORD win_time)
{
    return win_time - sg_clock_zero;
}

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

#elif defined(SG_CLOCK_POSIX_MONOTONIC)

struct timespec sg_clock_zero;

unsigned
sg_clock_convert(const struct timespec *ts)
{
    return (ts->tv_sec - sg_clock_zero.tv_sec) * 1000
        + (ts->tv_nsec - sg_clock_zero.tv_nsec) / 1000000;
}

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
    return sg_clock_convert(&tv);
}

#elif defined(SG_CLOCK_POSIX_SIMPLE)

struct timeval sg_clock_zero;

unsigned
sg_clock_convert(const struct timeval *tv)
{
    return (tv.tv_sec - sg_clock_zero.tv_sec) * 1000
        + (tv.tv_usec - sg_clock_zero.tv_usec) / 1000;
}

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
    return sg_clock_convert(&tv);
}

#else
#error "No clock implementation!"
#endif

static int
sg_clock_fmtdate(char *date, int year, int month, int day,
                 int hour, int minute, int sec, int msec)
{
    return snprintf(
        date, SG_DATE_LEN, "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ",
        year, month, day, hour, minute, sec, msec);
}

#if defined(_WIN32)

int
sg_clock_getdate(char *date)
{
    SYSTEMTIME t;
    GetSystemTime(&t);
    return sg_clock_fmtdate(date, t.wYear, t.wMonth, t.wDay,
                            t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);
}

#else
#include <sys/time.h>

int
sg_clock_getdate(char *date)
{
    struct timeval tv;
    struct tm tm;
    time_t t;
    gettimeofday(&tv, NULL);
    t = tv.tv_sec;
    gmtime_r(&t, &tm);
    return sg_clock_fmtdate(
        date, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
        tm.tm_hour, tm.tm_min, tm.tm_sec, tv.tv_usec / 1000);
}

#endif

#if defined(_WIN32)

void
sg_clock_sleep(unsigned milliseconds)
{
    Sleep(milliseconds);
}

#else

#include <time.h>

void
sg_clock_sleep(unsigned milliseconds)
{
    struct timespec req;
    req.tv_sec = milliseconds / 1000;
    req.tv_nsec = (milliseconds % 1000) * 1000000;
    nanosleep(&req, NULL);
}

#endif
