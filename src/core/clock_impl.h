/* Copyright 2012-2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "sg/defs.h"

/* Internal clock implementation.  These functions and variables are
   private to the base library, and are used for converting from OS
   timestamps to game timestamps.  */

#if defined __APPLE__
# define SG_CLOCK_APPLE 1

/* Convert a Mach timestamp to game time.  */
double
sg_clock_convert(uint64_t mach_time);

#elif defined _WIN32
# define SG_CLOCK_WINDOWS 1
# include <windows.h>

/* Convert a Windows timestamp (GetTickCount64) to game time.  */
double
sg_clock_convert(DWORD win_time);

/* Convert a Windows timestamp (GetTickCount64) to game time.  */
double
sg_clock_convert(DWORD win_time);

#else /* POSIX */
# include <time.h>
# include <unistd.h>

# if defined _POSIX_MONOTONIC_CLOCK
#  define SG_CLOCK_POSIX_MONOTONIC 1

/* Convert a POSIX monotonic clock timestamp to game time.  */
double
sg_clock_convert(const struct timespec *ts);

# else
#  define SG_CLOCK_POSIX_SIMPLE 1

/* Convert a POSIX timestamp to game time.  */
double
sg_clock_convert(const struct timeval *tv);

# endif /* POSIX MONOTONIC / SIMPLE */

#endif /* POSIX */
