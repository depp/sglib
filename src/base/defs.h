/* Various definitions needed for compatibility with various
   compilers (MSC) and platforms (Windows).  */
#ifndef BASE_DEFS_H
#define BASE_DEFS_H

#if __STDC_VERSION__ < 19901L
#if __GNUC__
#define restrict __restrict__
#else
#define restrict
#endif
#endif

#ifdef _MSC_VER

#if !defined(__cplusplus)
#define inline __inline
#endif

#define __attribute__(x)

#endif

#ifdef _WIN32
/* Target Windows XP */
#undef WINVER
#undef _WIN32_WINNT
#define WINVER 0x0501
#define _WIN32_WINNT 0x0501
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if defined(__linux__) || defined(__APPLE__)
#define HAVE_PTHREAD 1
#endif

#endif
