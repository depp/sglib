/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
/* Various definitions needed for compatibility with various compilers
   and platforms.  */
#ifndef SG_DEFS_H
#define SG_DEFS_H
#include "sg/attribute.h"

/* ========== Platforms ========== */

#if defined(_WIN32) /* Windows */
/* Target Windows XP */
# undef WINVER
# undef _WIN32_WINNT
# undef UNICODE
# define WINVER 0x0501
# define _WIN32_WINNT 0x0501
# define UNICODE 1
#elif defined(__linux__) /* Linux */
# define HAVE_PTHREAD 1
/* Target SuSv3 */
/* # define _XOPEN_SOURCE 600 */
# define _FILE_OFFSET_BITS 64
#elif defined(__APPLE__) /* OS X, iOS */
# define HAVE_PTHREAD 1
/* Target SuSv3 */
/* # define _XOPEN_SOURCE 600 */
#else
# error "Unknown platform"
#endif

/* ========== Error checks ========== */

#if !defined(SG_INLINE)
# warning "Unknown inline semantics"
#endif

/* ========== Config file ========== */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#endif
