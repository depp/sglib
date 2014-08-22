/* Copyright 2012-2013 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#ifndef SG_BYTEORDER_H
#define SG_BYTEORDER_H

/**
 * @file byteorder.h
 *
 * @brief Byte order macros.
 */

#if defined(__BYTE_ORDER__)
/* GCC 4.6 and later */
# define SG_BIG_ENDIAN __ORDER_BIG_ENDIAN__
# define SG_LITTLE_ENDIAN __ORDER_LITTLE_ENDIAN__
# define SG_BYTE_ORDER __BYTE_ORDER__
#else
/** @brief A constant used to indicate big endian.  */
# define SG_BIG_ENDIAN 4321
/** @brief A constant used to indicate little endian.  */
# define SG_LITTLE_ENDIAN 1234
# if defined(__BIG_ENDIAN__)
/* Clang and Apple's GCC */
#  define SG_BYTE_ORDER SG_BIG_ENDIAN
# elif defined(__LITTLE_ENDIAN__)
/* Clang and Apple's GCC */
#  define SG_BYTE_ORDER SG_LITTLE_ENDIAN
# elif defined(_M_IX86) || defined(__i386__) || defined(__i386) || \
    defined(__X86__) || defined(__I86__) || defined(__INTEL__) || \
    defined(_M_X64) || defined(__x86_64__) || \
    defined(__ARMEL__) || defined(__THUMBEL__) || \
    defined(_MIPSEL) || defined(__MIPSEL) || defined(__MIPSEL__)
#  define SG_BYTE_ORDER SG_LITTLE_ENDIAN
# elif defined(__ppc__) || defined(_M_PPC) || \
    defined(__ARMEB__) || defined(__THUMBEB__) ||\
    defined(_MIPSEB) || defined(__MIPSEB) || defined(__MIPSEB__)
#  define SG_BYTE_ORDER SG_BIG_ENDIAN
# elif defined(DOXYGEN)
/** @brief The target byte order, equal to either @c SG_BIG_ENDIAN or
    @c SG_LITTLE_ENDIAN.  */
#  define SG_BYTE_ORDER
# else
#  error "Unknown byte order"
# endif
#endif

#endif
