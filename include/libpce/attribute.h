/* Copyright 2012 Dietrich Epp <depp@zdome.net> */
/*
  Portable function attribute definitons.

  Most of these are available when using GCC and Clang, but some are
  also available when using Microsoft's compiler.

  PCE_ATTR_ARTIFICIAL for inline functions, debug locations in caller
  PCE_ATTR_CONST only examines arguments, no side effects
  PCE_ATTR_FORMAT(a,b,c) GCC format attribute
  PCE_ATTR_MALLOC returns unaliased pointers
  PCE_ATTR_NOINLINE do not inline
  PCE_ATTR_NONNULL((x,...)) parameters at given indices must be nonnull
  PCE_ATTR_NORETURN function does not return
  PCE_ATTR_NOTHROW function does not throw
  PCE_ATTR_PURE only examines arguments and globals, no side effects
  PCE_ATTR_SENTINEL function takes a NULL sentinel
  PCE_ATTR_WARN_UNUSED_RESULT warn if function result is unused
*/
#ifndef PCE_ATTRIBUTE_H
#define PCE_ATTRIBUTE_H

/*
  GCC attributes we don't have macros for:

  alias, aligned, alloc_size, always_inline, cold, constructor, error,
  externally_visible, flatten, format_arg, gnu_inline, hot, ifunc,
  no_instrument_function, no_split_stack, noclone, returns_twice,
  section, unused, used, warning, weak
*/

#if defined(__clang__)

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

#elif defined(__GNUC__)

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

# if __GNUC__ > 4 || (__GNUC_MINOR__ >= 3)
#  define PCE_ATTR_ARTIFICIAL __attribute__((__artificial__))
# endif

#elif defined(_MSC_VER)

# define PCE_ATTR_DEPRECATED __declspec(deprecated)
# define PCE_ATTR_MALLOC __declspec(restrict)
# define PCE_ATTR_NOINLINE __declspec(noinline)
# define PCE_ATTR_NORETURN __declspec(noreturn)
# define PCE_ATTR_NOTHROW __declspec(nothrow)

#endif

#ifndef PCE_ATTR_ARTIFICIAL
# define PCE_ATTR_ARTIFICIAL
#endif
#ifndef PCE_ATTR_CONST
# define PCE_ATTR_CONST
#endif
#ifndef PCE_ATTR_DEPRECATED
# define PCE_ATTR_DEPRECATED
#endif
#ifndef PCE_ATTR_FORMAT
# define PCE_ATTR_FORMAT(a,b,c)
#endif
#ifndef PCE_ATTR_MALLOC
# define PCE_ATTR_MALLOC
#endif
#ifndef PCE_ATTR_NOINLINE
# define PCE_ATTR_NOINLINE
#endif
#ifndef PCE_ATTR_NONNULL
# define PCE_ATTR_NONNULL(x)
#endif
#ifndef PCE_ATTR_NORETURN
# define PCE_ATTR_NORETURN
#endif
#ifndef PCE_ATTR_NOTHROW
# define PCE_ATTR_NOTHROW
#endif
#ifndef PCE_ATTR_PURE
# define PCE_ATTR_PURE
#endif
#ifndef PCE_ATTR_SENTINEL
# define PCE_ATTR_SENTINEL
#endif
#ifndef PCE_ATTR_WARN_UNUSED_RESULT
# define PCE_ATTR_WARN_UNUSED_RESULT
#endif

#endif
