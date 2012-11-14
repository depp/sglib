/* Copyright 2012 Dietrich Epp <depp@zdome.net> */
/*
  Portable compile-time probing of byte order.

  Defines PCE_BYTE_ORDER, PCE_BIG_ENDIAN, and PCE_LITTLE_ENDIAN.

  PCE_BYTE_ORDER will be defined to PCE_BIG_ENDIAN or PCE_LITTLE_ENDIAN
  as appropriate.
*/
#ifndef PCE_BYTEORDER_H
#define PCE_BYTEORDER_H

#if defined(__BYTE_ORDER__)
/* GCC 4.6 and later */
# define PCE_BIG_ENDIAN __ORDER_BIG_ENDIAN__
# define PCE_LITTLE_ENDIAN __ORDER_LITTLE_ENDIAN__
# define PCE_BYTE_ORDER __BYTE_ORDER__
#else
# define PCE_BIG_ENDIAN 4321
# define PCE_LITTLE_ENDIAN 1234
# if defined(__BIG_ENDIAN__)
/* Clang and Apple's GCC */
#  define PCE_BYTE_ORDER PCE_BIG_ENDIAN
# elif defined(__LITTLE_ENDIAN__)
/* Clang and Apple's GCC */
#  define PCE_BYTE_ORDER PCE_LITTLE_ENDIAN
# elif defined(_M_IX86) || defined(__i386__) || defined(__i386) || \
    defined(__X86__) || defined(__I86__) || defined(__INTEL__) || \
    defined(_M_X64) || defined(__x86_64__) || \
    defined(__ARMEL__) || defined(__THUMBEL__) || \
    defined(_MIPSEL) || defined(__MIPSEL) || defined(__MIPSEL__)
#  define PCE_BYTE_ORDER PCE_LITTLE_ENDIAN
# elif defined(__ppc__) || defined(_M_PPC) || \
    defined(__ARMEB__) || defined(__THUMBEB__) ||\
    defined(_MIPSEB) || defined(__MIPSEB) || defined(__MIPSEB__)
#  define PCE_BYTE_ORDER PCE_BIG_ENDIAN
# else
#  error "Unknown byte order"
# endif
#endif

#endif
