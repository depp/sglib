/* Various definitions needed for compatibility with various compilers
   and platforms.  */
#ifndef BASE_DEFS_H
#define BASE_DEFS_H

/*
  restrict: same semantics as C99 restrict, a macro if necessary
  SG_INLINE: approx same semantics as C99 "inline"
  SG_EXTERN_INLINE: approx same semantics as C99 "extern inline"
*/

#define SG_NORETURN
#define restrict

/* ========== Compilers ========== */

/* Microsoft MSC */
#ifdef _MSC_VER
# define __attribute__(x)
# undef restrict
# define restrict __restrict
# undef SG_NORETURN
# define SG_NORETURN __declspec(noreturn)
# define SG_INLINE __inline
# define SG_EXTERN_INLINE __inline
#endif

/* GNU GCC, Clang */
#if defined(__GNUC__)
# undef restrict
# define restrict __restrict__
# undef SG_NORETURN
# define SG_NORETURN __attribute__((noreturn))
# if defined(__GNUC_STDC_INLINE__)
#  define SG_INLINE __inline__
#  define SG_EXTERN_INLINE extern __inline__
# else
#  define SG_INLINE extern __inline__
#  define SG_EXTERN_INLINE __inline__
# endif
#endif

/* ========== Languages ========== */

/* We don't require C99 (due to MSC) but we support it.  */
#if defined(__STDC_VERSION__)
# if __STDC_VERSION__ >= 199901L
#  undef SG_INLINE
#  undef SG_EXTERN_INLINE
#  undef restrict
#  define SG_INLINE inline
#  define SG_EXTERN_INLINE extern inline
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

#if !defined(SG_INLINE) || !defined(SG_EXTERN_INLINE)
# error "Unknown inline semantics"
#endif

/* ========== Config file ========== */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#endif
