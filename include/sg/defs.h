/* Copyright 2012-2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#ifndef SG_DEFS_H
#define SG_DEFS_H
#include "config.h"

/**
 * @file defs.h
 *
 * @brief Platform and compiler-specific macros.
 */

/* ======================================================================
   Platform macros
   ====================================================================== */

#if defined _WIN32 /* Windows */
/* Target Windows XP */
# undef WINVER
# undef _WIN32_WINNT
# undef UNICODE
# define WINVER 0x0501
# define _WIN32_WINNT 0x0501
# define UNICODE 1
#elif defined __linux__ /* Linux */
# define HAVE_PTHREAD 1
/* Target SuSv3 */
/* # define _XOPEN_SOURCE 600 */
# define _FILE_OFFSET_BITS 64
#elif defined __APPLE__ /* OS X, iOS */
# define HAVE_PTHREAD 1
/* Target SuSv3 */
/* # define _XOPEN_SOURCE 600 */
#else
# error "Unknown platform"
#endif

/* ======================================================================
   Architecture macros
   ====================================================================== */

#if defined DOXYGEN
/** @brief Defined for 64-bit x86 CPUs.  */
# define SG_CPU_X64 1
/** @brief Defined for x86 CPUs, including 64-bit.  */
# define SG_CPU_X86 1
/** @brief Defined for PowerPC 64-bit CPUs.  */
# define SG_CPU_PPC64 1
/** @brief Defined for PowerPC CPUs, including 64-bit.  */
# define SG_CPU_PPC 1
/** @brief Defined for ARM CPUs.  */
# define SG_CPU_ARM 1
#endif

#if defined _M_X64 || defined __x86_64__
# define SG_CPU_X64 1
# define SG_CPU_X86 1
#elif defined _M_IX86 || defined __i386__ || defined __i386 || \
    defined __X86__ || defined __I86__ || defined __INTEL__
# define SG_CPU_X86 1
#elif defined __ppc64__
# define SG_CPU_PPC64 1
# define SG_CPU_PPC 1
#elif defined __ppc__ || defined __powerpc__
# define SG_CPU_PPC 1
#elif defined __arm__ || defined __thumb__
# define SG_CPU_ARM 1
#endif

/* ======================================================================
   Byte order macros
   ====================================================================== */

#if defined __BYTE_ORDER__
/* GCC 4.6 and later */
# define SG_BIG_ENDIAN __ORDER_BIG_ENDIAN__
# define SG_LITTLE_ENDIAN __ORDER_LITTLE_ENDIAN__
# define SG_BYTE_ORDER __BYTE_ORDER__
#else
/** @brief A constant used to indicate big endian.  */
# define SG_BIG_ENDIAN 4321
/** @brief A constant used to indicate little endian.  */
# define SG_LITTLE_ENDIAN 1234
# if defined __BIG_ENDIAN__
/* Clang and Apple's GCC */
#  define SG_BYTE_ORDER SG_BIG_ENDIAN
# elif defined __LITTLE_ENDIAN__
/* Clang and Apple's GCC */
#  define SG_BYTE_ORDER SG_LITTLE_ENDIAN
# elif defined _M_IX86 || defined __i386__ || defined __i386 || \
    defined __X86__ || defined __I86__ || defined __INTEL__ || \
    defined _M_X64 || defined __x86_64__ || \
    defined __ARMEL__ || defined __THUMBEL__ || \
    defined _MIPSEL || defined __MIPSEL || defined __MIPSEL__
#  define SG_BYTE_ORDER SG_LITTLE_ENDIAN
# elif defined __ppc__ || defined _M_PPC || \
    defined __ARMEB__ || defined __THUMBEB__ ||\
    defined _MIPSEB || defined __MIPSEB || defined __MIPSEB__
#  define SG_BYTE_ORDER SG_BIG_ENDIAN
# elif defined DOXYGEN
/** @brief The target byte order, equal to either @c SG_BIG_ENDIAN or
    @c SG_LITTLE_ENDIAN.  */
#  define SG_BYTE_ORDER
# else
#  error "Unknown byte order"
# endif
#endif

/*
  GCC attributes we don't have macros for:

  alias, aligned, alloc_size, always_inline, cold, constructor, error,
  externally_visible, flatten, format_arg, gnu_inline, hot, ifunc,
  no_instrument_function, no_split_stack, noclone, returns_twice,
  section, unused, used, warning, weak
*/

#if defined __clang__

/* ======================================================================
   Clang
   ====================================================================== */

/* Must be before GCC, because Clang pretends to be GCC.  */

# if __has_attribute(__artificial__)
#  define SG_ATTR_CONST __attribute__((__artificial__))
# endif
/* Testing for const with __has_attribute() is broken, it seems */
# define SG_ATTR_CONST __attribute__((__const__))
# if __has_attribute(__deprecated__)
#  define SG_ATTR_DEPRECATED __attribute__((__deprecated__))
# endif
# if __has_attribute(__format__)
#  define SG_ATTR_FORMAT(a,b,c) __attribute__((format(a,b,c)))
# endif
# if __has_attribute(__malloc__)
#  define SG_ATTR_MALLOC __attribute__((__malloc__))
# endif
# if __has_attribute(__noinline__)
#  define SG_ATTR_NOINLINE __attribute__((__noinline__))
# endif
# if __has_attribute(__nonnull__)
#  define SG_ATTR_NONNULL(x) __attribute__((__nonnull__ x))
# endif
# if __has_attribute(__noreturn__)
#  define SG_ATTR_NORETURN __attribute__((__noreturn__))
# endif
# if __has_attribute(__nothrow__)
#  define SG_ATTR_NOTHROW __attribute__((__nothrow__))
# endif
# if __has_attribute(__pure__)
#  define SG_ATTR_PURE __attribute__((__pure__))
# endif
# if __has_attribute(__sentinel__)
#  define SG_ATTR_SENTINEL __attribute__((__sentinel__))
# endif
# if __has_attribute(__warn_unused_result__)
#  define SG_ATTR_WARN_UNUSED_RESULT __attribute__((__warn_unused_result__))
# endif

# define SG_INLINE static __inline__
# define SG_RESTRICT __restrict__

#elif defined __GNUC__

/* ======================================================================
   GCC
   ====================================================================== */

# if __GNUC__ >= 3
#  define SG_ATTR_CONST __attribute__((__const__))
#  define SG_ATTR_FORMAT(a,b,c) __attribute__((__format__(a,b,c)))
#  define SG_ATTR_MALLOC __attribute__((__malloc__))
#  define SG_ATTR_NORETURN __attribute__((__noreturn__))
#  define SG_ATTR_PURE __attribute__((__pure__))
# endif

# if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1)
#  define SG_ATTR_DEPRECATED __attribute__((__deprecated__))
#  define SG_ATTR_NOINLINE __attribute__((__noinline__))
# endif

# if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 3)
#  define SG_ATTR_NONNULL(x) __attribute__((__nonnull__ x))
#  define SG_ATTR_NOTHROW __attribute__((__nothrow__))
# endif

# if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)
#  define SG_ATTR_WARN_UNUSED_RESULT __attribute__((__warn_unused_result__))
# endif

# if __GNUC__ >= 4
#  define SG_ATTR_SENTINEL __attribute__((__sentinel__))
# endif

# if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3)
#  define SG_ATTR_ARTIFICIAL __attribute__((__artificial__))
# endif

# define SG_INLINE static __inline__

#elif defined _MSC_VER

/* ======================================================================
   Visual Studio
   ====================================================================== */

# define SG_ATTR_DEPRECATED __declspec(deprecated)
# define SG_ATTR_MALLOC __declspec(restrict)
# define SG_ATTR_NOINLINE __declspec(noinline)
# define SG_ATTR_NORETURN __declspec(noreturn)
# define SG_ATTR_NOTHROW __declspec(nothrow)

# define SG_INLINE static __inline
# define SG_RESTRICT __restrict

#endif

/* ======================================================================
   C99 and C11
   ====================================================================== */

/* We don't require C99 (due to MSC) but we support it.  */
#if defined __STDC_VERSION__
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

/* ======================================================================
   Defaults
   ====================================================================== */

#ifndef SG_ATTR_ARTIFICIAL
/** @brief for inline functions, debug locations in caller */
# define SG_ATTR_ARTIFICIAL
#endif
#ifndef SG_ATTR_CONST
/** @brief only examines arguments, no side effects */
# define SG_ATTR_CONST
#endif
#ifndef SG_ATTR_DEPRECATED
/** @brief tihs function is deprecated, using it will generate a warning */
# define SG_ATTR_DEPRECATED
#endif
#ifndef SG_ATTR_FORMAT
/** @brief GCC format attribute */
# define SG_ATTR_FORMAT(a,b,c)
#endif
#ifndef SG_ATTR_MALLOC
/** @brief returns unaliased pointers */
# define SG_ATTR_MALLOC
#endif
#ifndef SG_ATTR_NOINLINE
/** @brief do not inline */
# define SG_ATTR_NOINLINE
#endif
#ifndef SG_ATTR_NONNULL
/** @brief parameters at given indices must be nonnull */
# define SG_ATTR_NONNULL(x)
#endif
#ifndef SG_ATTR_NORETURN
/** @brief function does not return */
# define SG_ATTR_NORETURN
#endif
#ifndef SG_ATTR_NOTHROW
/** @brief function does not throw */
# define SG_ATTR_NOTHROW
#endif
#ifndef SG_ATTR_PURE
/** @brief only examines arguments and globals, no side effects */
# define SG_ATTR_PURE
#endif
#ifndef SG_ATTR_SENTINEL
/** @brief function takes a NULL sentinel */
# define SG_ATTR_SENTINEL
#endif
#ifndef SG_ATTR_WARN_UNUSED_RESULT
/** @brief warn if function result is unused */
# define SG_ATTR_WARN_UNUSED_RESULT
#endif

#if defined DOXYGEN
/** @brief function can be inlined by the compiler */
# define SG_INLINE
#endif

#ifndef SG_RESTRICT
/** @brief Object has no aliases */
# define SG_RESTRICT
#endif

#if !defined SG_INLINE
# warning "Unknown inline semantics"
#endif

#endif
