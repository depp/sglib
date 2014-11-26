/* Copyright 2012-2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "clock_impl.h"
#include "sg/clock.h"
#include "private.h"
#include <stdio.h>

#if defined SG_CLOCK_SDL
#include "SDL.h"

double
sg_clock_convert(unsigned sdl_time)
{
    return 0.001 * sdl_time;
}

void
sg_clock_init(void)
{ }

double
sg_clock_get(void)
{
    return 0.001 * SDL_GetTicks();
}

#elif defined SG_CLOCK_APPLE
/* ===================================================================
   OS X (Mach)
   =================================================================== */
#include <mach/mach_time.h>
#include <time.h>

static struct {
    uint64_t zero;
    double scale;
} sg_clock;

double
sg_clock_convert(uint64_t mach_time)
{
    return (double) (mach_time - sg_clock.zero) * sg_clock.scale;
}

void
sg_clock_init(void)
{
    struct mach_timebase_info info;
    mach_timebase_info(&info);
    sg_clock.zero = mach_absolute_time();
    sg_clock.scale = (double) info.numer / ((double) info.denom * 1e9);
}

double
sg_clock_get(void)
{
    return sg_clock_convert(mach_absolute_time());
}

#elif defined SG_CLOCK_WINDOWS
/* ===================================================================
   Windows
   =================================================================== */

static DWORD sg_clock_zero;

double
sg_clock_convert(DWORD win_time)
{
    return (double) (win_time - sg_clock_zero);
}

void
sg_clock_init(void)
{
    sg_clock_zero = GetTickCount();
}

double
sg_clock_get(void)
{
    return sg_clock_convert(GetTickCount());
}

#elif defined SG_CLOCK_POSIX_MONOTONIC
/* ===================================================================
   POSIX with CLOCK_MONOTONIC
   =================================================================== */

struct timespec sg_clock_zero;

double
sg_clock_convert(const struct timespec *ts)
{
    return (double) (ts->tv_sec - sg_clock_zero.tv_sec) +
        + (double) (ts->tv_nsec - sg_clock_zero.tv_nsec) * 1e-9;
}

void
sg_clock_init(void)
{
    clock_gettime(CLOCK_MONOTONIC, &sg_clock_zero);
}

double
sg_clock_get(void)
{
    struct timespec tv;
    clock_gettime(CLOCK_MONOTONIC, &tv);
    return sg_clock_convert(&tv);
}

#elif defined SG_CLOCK_POSIX_SIMPLE
/* ===================================================================
   POSIX without CLOCK_MONOTONIC
   =================================================================== */

struct timeval sg_clock_zero;

double
sg_clock_convert(const struct timeval *tv)
{
    return (double) (tv.tv_sec - sg_clock_zero.tv_sec) *
        + (double)(tv.tv_usec - sg_clock_zero.tv_usec) * 1e-6;
}

void
sg_clock_init(void)
{
    gettimeofday(&sg_clock_zero, NULL);
}

double
sg_clock_get(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return sg_clock_convert(&tv);
}

#else
#error "No clock implementation!"
#endif

/* ===================================================================
   Calendar date
   =================================================================== */

static int
sg_clock_fmtdate(char *date, int shortfmt, int year, int month, int day,
                 int hour, int minute, int sec, int msec)
{
    const char *fmt = shortfmt ?
        "%04d%02d%02dT%02d%02d%02d%03d" :
        "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ";
#define ARGS year, month, day, hour, minute, sec, msec
#if !defined _WIN32
    return snprintf(date, SG_DATE_LEN, fmt, ARGS);
#else
    return _snprintf_s(date, SG_DATE_LEN, _TRUNCATE, fmt, ARGS);
#endif
}

#if defined _WIN32

int
sg_clock_getdate(char *date, int shortfmt)
{
    SYSTEMTIME t;
    GetSystemTime(&t);
    return sg_clock_fmtdate(date, shortfmt, t.wYear, t.wMonth, t.wDay,
                            t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);
}

#else
#include <sys/time.h>
#include <time.h>

int
sg_clock_getdate(char *date, int shortfmt)
{
    struct timeval tv;
    struct tm tm;
    time_t t;
    gettimeofday(&tv, NULL);
    t = tv.tv_sec;
    gmtime_r(&t, &tm);
    return sg_clock_fmtdate(
        date, shortfmt, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
        tm.tm_hour, tm.tm_min, tm.tm_sec, tv.tv_usec / 1000);
}

#endif
