/* Copyright 2012-2013 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#ifndef SG_ATTRIBUTE_H
#define SG_ATTRIBUTE_H

/**
 * @file attribute.h
 *
 * @brief Portable function attribute definitions.
 *
 * Most of these are available when using GCC or Clang, but a few are
 * also available when using Visual C++.
 */

/*
  GCC attributes we don't have macros for:

  alias, aligned, alloc_size, always_inline, cold, constructor, error,
  externally_visible, flatten, format_arg, gnu_inline, hot, ifunc,
  no_instrument_function, no_split_stack, noclone, returns_twice,
  section, unused, used, warning, weak
*/

#if defined(__clang__)

/*
  ============================================================
  Clang
*/

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

#elif defined(__GNUC__)

/*
  ============================================================
  GCC
*/

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

#elif defined(_MSC_VER)

/*
  ============================================================
  Visual C++
*/

# define SG_ATTR_DEPRECATED __declspec(deprecated)
# define SG_ATTR_MALLOC __declspec(restrict)
# define SG_ATTR_NOINLINE __declspec(noinline)
# define SG_ATTR_NORETURN __declspec(noreturn)
# define SG_ATTR_NOTHROW __declspec(nothrow)

# define SG_INLINE static __inline
# define SG_RESTRICT __restrict

#endif

/*
  ============================================================
  Use C99 or C11 where available.
*/

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

/*
  ============================================================
  Defaults
*/

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

#if defined(DOXYGEN)
/** @brief function can be inlined by the compiler */
# define SG_INLINE
#endif

#ifndef SG_RESTRICT
/** @brief Object has no aliases */
# define SG_RESTRICT
#endif

#endif
