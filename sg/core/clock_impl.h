/* Copyright 2012 Dietrich Epp <depp@zdome.net> */
/* Internal clock implementation.  These functions and variables are
   private to the base library, and are used for converting from OS
   timestamps to game timestamps.

   Each platform will export two important pieces:

   sg_clock_zero: the OS timestamp of the game's epoch.  The game
   always initializes this so that the epoch is when the game
   launches.

   sg_clock_convert: convert the OS timestamp to the number of
   milliseconds since the game epoch.  */
#ifndef SG_CORE_CLOCK_IMPL_H
#define SG_CORE_CLOCK_IMPL_H

#if defined(__APPLE__)
#define SG_CLOCK_APPLE 1
#include <mach/mach_time.h>
#include <time.h>

extern uint64_t sg_clock_zero;
extern struct mach_timebase_info sg_clock_info;

/* Convert a Mach timestamp to game milliseconds.  */
unsigned
sg_clock_convert(uint64_t mach_time);

#elif defined(_WIN32)
#define SG_CLOCK_WINDOWS 1
#include <windows.h>

extern DWORD sg_clock_zero;

/* Convert a Windows timestamp (GetTickCount) to game
   milliseconds.  */
unsigned
sg_clock_convert(DWORD win_time);

#else /* POSIX */
#include <time.h>
#include <unistd.h>

#if _POSIX_TIMERS && defined(_POSIX_MONOTONIC_CLOCK)
#define SG_CLOCK_POSIX_MONOTONIC 1

extern struct timespec sg_clock_zero;

/* Convert a POSIX monotonic clock timestamp to game milliseconds.  */
unsigned
sg_clock_convert(const struct timespec *ts);

#else
#define SG_CLOCK_POSIX_SIMPLE 1

extern struct timeval sg_clock_zero;

/* Convert a POSIX timestamp to game milliseconds.  */
unsigned
sg_clock_convert(const struct timeval *tv);

#endif /* POSIX MONOTONIC / SIMPLE */

#endif /* POSIX */

#endif
