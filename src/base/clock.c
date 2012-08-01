#include "clock.h"
#include "clock_impl.h"

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

static void
sg_clock_fmtfield(char *p, int width, int n)
{
    int i;
    for (i = 0; i < width; ++i) {
        p[width - 1 - i] = '0' + (n % 10);
        n /= 10;
    }
}

static int
sg_clock_fmtdate(char *date, int year, int month, int day,
                 int hour, int minute, int sec, int msec)
{
    sg_clock_fmtfield(date, 4, year);
    date[4] = '-';
    sg_clock_fmtfield(date + 5, 2, month);
    date[7] = '-';
    sg_clock_fmtfield(date + 8, 2, day);
    date[10] = 'T';
    sg_clock_fmtfield(date + 11, 2, hour);
    date[13] = ':';
    sg_clock_fmtfield(date + 14, 2, minute);
    date[16] = ':';
    sg_clock_fmtfield(date + 17, 2, sec);
    date[19] = '.';
    sg_clock_fmtfield(date + 20, 3, msec);
    date[23] = 'Z';
    return 24;
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
