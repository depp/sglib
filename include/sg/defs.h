/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
/* Various definitions needed for compatibility with various compilers
   and platforms.  */
#ifndef SG_DEFS_H
#define SG_DEFS_H

/*
  SG_RESTRICT: same semantics as C99 "restrict," or expands to nothing
  SG_INLINE: approx same semantics as C99 "static inline"
  SG_NORETURN: marks a function as not returning
*/

#define SG_NORETURN
#define SG_RESTRICT

/* ========== Compilers ========== */

/* Microsoft MSC */
#ifdef _MSC_VER
# define __attribute__(x)
# undef SG_RESTRICT
# define SG_RESTRICT __restrict
# undef SG_NORETURN
# define SG_NORETURN __declspec(noreturn)
# undef SG_INLINE
# define SG_INLINE static __inline
#endif

/* GNU GCC, Clang */
#if defined(__GNUC__)
# undef SG_RESTRICT
# define SG_RESTRICT __restrict__
# undef SG_NORETURN
# define SG_NORETURN __attribute__((noreturn))
# undef SG_INLINE
# define SG_INLINE static __inline__
#endif

/* ========== Languages ========== */

/* We don't require C99 (due to MSC) but we support it.  */
#if defined(__STDC_VERSION__)
# if __STDC_VERSION__ >= 199901L
#  undef SG_INLINE
#  undef SG_RESTRICT
#  define SG_RESTRICT restrict
#  define SG_INLINE static inline
#  if __STDC_VERSION__ >= 201112L
#   undef SG_NORETURN
#   define SG_NORETURN _Noreturn
#  endif
# endif
#endif

/* ========== Platforms ========== */

#if defined(_WIN32) /* Windows */
/* Target Windows XP */
# undef WINVER
# undef _WIN32_WINNT
# define WINVER 0x0501
# define _WIN32_WINNT 0x0501
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
