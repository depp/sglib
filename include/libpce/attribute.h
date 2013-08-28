/* Copyright 2012-2013 Dietrich Epp <depp@zdome.net> */
#ifndef PCE_ATTRIBUTE_H
#define PCE_ATTRIBUTE_H

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
#  define PCE_ATTR_CONST __attribute__((__artificial__))
# endif
/* Testing for const with __has_attribute() is broken, it seems */
# define PCE_ATTR_CONST __attribute__((__const__))
# if __has_attribute(__deprecated__)
#  define PCE_ATTR_DEPRECATED __attribute__((__deprecated__))
# endif
# if __has_attribute(__format__)
#  define PCE_ATTR_FORMAT(a,b,c) __attribute__((format(a,b,c)))
# endif
# if __has_attribute(__malloc__)
#  define PCE_ATTR_MALLOC __attribute__((__malloc__))
# endif
# if __has_attribute(__noinline__)
#  define PCE_ATTR_NOINLINE __attribute__((__noinline__))
# endif
# if __has_attribute(__nonnull__)
#  define PCE_ATTR_NONNULL(x) __attribute__((__nonnull__ x))
# endif
# if __has_attribute(__noreturn__)
#  define PCE_ATTR_NORETURN __attribute__((__noreturn__))
# endif
# if __has_attribute(__nothrow__)
#  define PCE_ATTR_NOTHROW __attribute__((__nothrow__))
# endif
# if __has_attribute(__pure__)
#  define PCE_ATTR_PURE __attribute__((__pure__))
# endif
# if __has_attribute(__sentinel__)
#  define PCE_ATTR_SENTINEL __attribute__((__sentinel__))
# endif
# if __has_attribute(__warn_unused_result__)
#  define PCE_ATTR_WARN_UNUSED_RESULT __attribute__((__warn_unused_result__))
# endif

# define PCE_INLINE static __inline__

#elif defined(__GNUC__)

/*
  ============================================================
  GCC
*/

# if __GNUC__ >= 3
#  define PCE_ATTR_CONST __attribute__((__const__))
#  define PCE_ATTR_FORMAT(a,b,c) __attribute__((__format__(a,b,c)))
#  define PCE_ATTR_MALLOC __attribute__((__malloc__))
#  define PCE_ATTR_NORETURN __attribute__((__noreturn__))
#  define PCE_ATTR_PURE __attribute__((__pure__))
# endif

# if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1)
#  define PCE_ATTR_DEPRECATED __attribute__((__deprecated__))
#  define PCE_ATTR_NOINLINE __attribute__((__noinline__))
# endif

# if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 3)
#  define PCE_ATTR_NONNULL(x) __attribute__((__nonnull__ x))
#  define PCE_ATTR_NOTHROW __attribute__((__nothrow__))
# endif

# if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)
#  define PCE_ATTR_WARN_UNUSED_RESULT __attribute__((__warn_unused_result__))
# endif

# if __GNUC__ >= 4
#  define PCE_ATTR_SENTINEL __attribute__((__sentinel__))
# endif

# if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3)
#  define PCE_ATTR_ARTIFICIAL __attribute__((__artificial__))
# endif

# define PCE_INLINE static __inline__

#elif defined(_MSC_VER)

/*
  ============================================================
  Visual C++
*/

# define PCE_ATTR_DEPRECATED __declspec(deprecated)
# define PCE_ATTR_MALLOC __declspec(restrict)
# define PCE_ATTR_NOINLINE __declspec(noinline)
# define PCE_ATTR_NORETURN __declspec(noreturn)
# define PCE_ATTR_NOTHROW __declspec(nothrow)

# define PCE_INLINE static __inline

#endif

/*
  ============================================================
  Defaults
*/

#ifndef PCE_ATTR_ARTIFICIAL
/** @brief for inline functions, debug locations in caller */
# define PCE_ATTR_ARTIFICIAL
#endif
#ifndef PCE_ATTR_CONST
/** @brief only examines arguments, no side effects */
# define PCE_ATTR_CONST
#endif
#ifndef PCE_ATTR_DEPRECATED
/** @brief tihs function is deprecated, using it will generate a warning */
# define PCE_ATTR_DEPRECATED
#endif
#ifndef PCE_ATTR_FORMAT
/** @brief GCC format attribute */
# define PCE_ATTR_FORMAT(a,b,c)
#endif
#ifndef PCE_ATTR_MALLOC
/** @brief returns unaliased pointers */
# define PCE_ATTR_MALLOC
#endif
#ifndef PCE_ATTR_NOINLINE
/** @brief do not inline */
# define PCE_ATTR_NOINLINE
#endif
#ifndef PCE_ATTR_NONNULL
/** @brief parameters at given indices must be nonnull */
# define PCE_ATTR_NONNULL(x)
#endif
#ifndef PCE_ATTR_NORETURN
/** @brief function does not return */
# define PCE_ATTR_NORETURN
#endif
#ifndef PCE_ATTR_NOTHROW
/** @brief function does not throw */
# define PCE_ATTR_NOTHROW
#endif
#ifndef PCE_ATTR_PURE
/** @brief only examines arguments and globals, no side effects */
# define PCE_ATTR_PURE
#endif
#ifndef PCE_ATTR_SENTINEL
/** @brief function takes a NULL sentinel */
# define PCE_ATTR_SENTINEL
#endif
#ifndef PCE_ATTR_WARN_UNUSED_RESULT
/** @brief warn if function result is unused */
# define PCE_ATTR_WARN_UNUSED_RESULT
#endif

#if defined(DOXYGEN)
/** @brief function can be inlined by the compiler */
# define PCE_INLINE
#endif

#endif
