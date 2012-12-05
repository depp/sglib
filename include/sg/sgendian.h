/* Defines BIG_ENDIAN, LITTLE_ENDIAN, and BYTE_ORDER.  Tries to work
   on as many platforms as possible.  Uses the definitions from a
   system-provided header file when it is available.  Otherwise, uses
   builtin preprocessor macros if available.  As a last resort, checks
   preprocessor macros for the machine type.

   Don't try to detect byte order in a configure script, unless you
   are sure you can get it right.  Mac OS X supports compilation for
   multiple architectures *simultaneously* -- that is, calling "gcc"
   once will give you a fat object file with code for both i386 and
   PowerPC.  (I think the favored technique for multiple architecture
   compilation is to run the compiler twice and combine the results,
   but who knows).

   This really should work practically everywhere, except crazy
   systems like ones with mixed endian.

   In order to avoid collisions with system headers, this is not named
   "endian.h".  */
#ifndef BASE_SGENDIAN_H
#define BASE_SGENDIAN_H

#if defined(__linux__)
#include <endian.h>
#ifndef BYTE_ORDER
#define LITTLE_ENDIAN __LITTLE_ENDIAN
#define BIG_ENDIAN __BIG_ENDIAN
#define BYTE_ORDER __BYTE_ORDER
#endif
#elif defined(__APPLE__)
#include <machine/endian.h>
#else

#define BIG_ENDIAN 1234
#define LITTLE_ENDIAN 4321

#if defined(__BIG_ENDIAN__) && __BIG_ENDIAN__
#define BYTE_ORDER BIG_ENDIAN
#elif defined(__LITTLE_ENDIAN__) && __LITTLE_ENDIAN__
#define BYTE_ORDER LITTLE_ENDIAN
/* This line is taken from LibSDL's SDL_endian.h header file.  */
#elif defined(__hppa__) || \
    defined(__m68k__) || defined(mc68000) || defined(_M_M68K) || \
    (defined(__MIPS__) && defined(__MISPEB__)) || \
    defined(__ppc__) || defined(__POWERPC__) || defined(_M_PPC) || \
    defined(__sparc__)
#define BYTE_ORDER BIG_ENDIAN
#else
#define BYTE_ORDER LITTLE_ENDIAN
#endif

#endif

#endif
