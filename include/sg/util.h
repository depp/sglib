/* Copyright 2012-2013 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#ifndef SG_UTIL_H
#define SG_UTIL_H
#ifdef __cplusplus
extern "C" {
#endif
#include <stddef.h>
#include "sg/attribute.h"
#include "sg/byteorder.h"

/**
 * @file util.h
 *
 * @brief Utility functions.
 *
 * These utility functions are miscellaneous useful integer functions.
 * This includes byte swapping, rotations, and alignmment.
 */

#ifdef DOXYGEN
/**
 * @brief It is here
 */
void it_is_here();
#endif

#if defined(__clang__)
# if __has_builtin(__builtin_bswap32)
#  define SG_SWAP32 __builtin_bswap32
# endif
# if __has_builtin(__builtin_bswap64)
#  define SG_SWAP64 __builtin_bswap64
# endif
#elif defined(__GNUC__)
# if __GNUC__ > 4 || (__GNUC__ >= 4 && __GNUC_MINOR__ >= 3)
#  define SG_SWAP32 __builtin_bswap32
#  define SG_SWAP64 __builtin_bswap64
# endif
#elif defined(_MSC_VER)
# include <stdlib.h>
# define SG_SWAP16 _byteswap_ushort
# define SG_SWAP32 _byteswap_ulong
# define SG_SWAP64 _byteswap_uint64
#endif

/**
 * @brief Swap the byte order of a 16-bit integer.
 */
SG_ATTR_ARTIFICIAL SG_ATTR_CONST SG_INLINE
unsigned short
sg_swap16(unsigned short x);

/**
 * @brief Swap the byte order of a 32-bit integer.
 */
SG_ATTR_ARTIFICIAL SG_ATTR_CONST SG_INLINE
unsigned
sg_swap32(unsigned x);

/**
 * @brief Swap the byte order of a 64-bit integer.
 */
SG_ATTR_ARTIFICIAL SG_ATTR_CONST SG_INLINE
unsigned long long
sg_swap64(unsigned long long x);

#if defined(SG_SWAP16)
SG_ATTR_ARTIFICIAL SG_ATTR_CONST SG_INLINE
unsigned short
sg_swap16(unsigned short x)
{
    return SG_SWAP16(x);
}
#elif defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
SG_ATTR_ARTIFICIAL SG_ATTR_CONST SG_INLINE
unsigned short
sg_swap16(unsigned short x)
{
    __asm__("xchgb %b0,%h0" : "=Q"(x) : "0"(x));
    return x;
}
#elif defined(__GNUC__) && (defined(__powersg__) || defined(__psg__))
SG_ATTR_ARTIFICIAL SG_ATTR_CONST SG_INLINE
unsigned short
sg_swap16(unsigned short x)
{
    unsigned tmp;
    __asm__("rlwimi %0,%2,8,16,23"
            : "=r"(tmp)
            : "0"(x >> 8), "r"(x));
    return (unsigned short) tmp;
}
#else
SG_ATTR_ARTIFICIAL SG_ATTR_CONST SG_INLINE
unsigned short
sg_swap16(unsigned short x)
{
    return (unsigned short) ((x >> 8) | (x << 8));
}
#endif

#if defined(SG_SWAP32)
SG_ATTR_ARTIFICIAL SG_ATTR_CONST SG_INLINE
unsigned
sg_swap32(unsigned x)
{
    return SG_SWAP32(x);
}
#elif defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
SG_ATTR_ARTIFICIAL SG_ATTR_CONST SG_INLINE
unsigned
sg_swap32(unsigned x)
{
    __asm__("bswap %0" : "=r"(x) : "0"(x));
    return x;
}
#elif defined(__GNUC__) && (defined(__powersg__) || defined(__psg__))
SG_ATTR_ARTIFICIAL SG_ATTR_CONST SG_INLINE
unsigned
sg_swap32(unsigned x)
{
    unsigned tmp;
    __asm__("rotlwi %0,%1,8" : "=r"(tmp) : "r"(x));
    __asm__("rlwimi %0,%2,24,0,7" : "=r"(tmp) : "0"(tmp), "r"(x));
    __asm__("rlwimi %0,%2,24,16,23" : "=r"(tmp) : "0"(tmp), "r"(x));
    return tmp;
}
#else
SG_ATTR_ARTIFICIAL SG_ATTR_CONST SG_INLINE
unsigned
sg_swap32(unsigned x)
{
    return
        ((x & 0xff000000) >> 24) |
        ((x & 0x00ff0000) >> 8) |
        ((x & 0x0000ff00) << 8) |
        ((x & 0x000000ff) << 24);
}
#endif

#if defined(SG_SWAP64)
SG_ATTR_ARTIFICIAL SG_ATTR_CONST SG_INLINE
unsigned long long
sg_swap64(unsigned long long x)
{
    return SG_SWAP64(x);
}
#elif defined(__GNUC__) && defined(__x86_64__)
SG_ATTR_ARTIFICIAL SG_ATTR_CONST SG_INLINE
unsigned long long
sg_swap64(unsigned long long x)
{
    __asm__("bswapq %0" : "=r"(x) : "0"(x));
}
#else
SG_ATTR_ARTIFICIAL SG_ATTR_CONST SG_INLINE
unsigned long long
sg_swap64(unsigned long long x)
{
    return
        (unsigned long long) sg_swap32((unsigned) (x >> 32)) |
        ((unsigned long long) sg_swap32((unsigned) x) << 32);
}
#endif

#if SG_BYTE_ORDER == SG_LITTLE_ENDIAN
# define sg_swapbe16(x) sg_swap16(x)
# define sg_swapbe32(x) sg_swap32(x)
# define sg_swapbe64(x) sg_swap64(x)
# define sg_swaple16(x) (x)
# define sg_swaple32(x) (x)
# define sg_swaple64(x) (x)
#elif SG_BYTE_ORDER == SG_BIG_ENDIAN
# define sg_swapbe16(x) (x)
# define sg_swapbe32(x) (x)
# define sg_swapbe64(x) (x)
# define sg_swaple16(x) sg_swap16(x)
# define sg_swaple32(x) sg_swap32(x)
# define sg_swaple64(x) sg_swap64(x)
#elif defined(DOXYGEN)
/** @brief Swap the byte order of a 16-bit integer between native and
    big endian.  */
# define sg_swapbe16(x)
/** @brief Swap the byte order of a 32-bit integer between native and
    big endian.  */
# define sg_swapbe32(x)
/** @brief Swap the byte order of a 64-bit integer between native and
    big endian.  */
# define sg_swapbe64(x)
/** @brief Swap the byte order of a 16-bit integer between native and
    little endian.  */
# define sg_swaple16(x)
/** @brief Swap the byte order of a 32-bit integer between native and
    little endian.  */
# define sg_swaple32(x)
/** @brief Swap the byte order of a 64-bit integer between native and
    little endian.  */
# define sg_swaple64(x)
#endif

/**
 * @brief Rotate a 32-bit integer to the left.
 */
SG_ATTR_ARTIFICIAL SG_ATTR_CONST SG_INLINE
unsigned
sg_rotl_32(unsigned x, unsigned k);

/**
 * @brief Rotate a 32-bit integer to the right.
 */
SG_ATTR_ARTIFICIAL SG_ATTR_CONST SG_INLINE
unsigned
sg_rotr_32(unsigned x, unsigned k);

#if defined(_MSC_VER)

#include <stdlib.h>

SG_ATTR_ARTIFICIAL SG_ATTR_CONST SG_INLINE
unsigned
sg_rotl_32(unsigned x, unsigned k)
{
    return _rotl(x, k);
}

SG_ATTR_ARTIFICIAL SG_ATTR_CONST SG_INLINE
unsigned
sg_rotr_32(unsigned x, unsigned k)
{
    return _rotr(x, k);
}

#else

SG_ATTR_ARTIFICIAL SG_ATTR_CONST SG_INLINE
unsigned
sg_rotl_32(unsigned x, unsigned k)
{
    return (x << k) | (x >> (32 - k));
}

SG_ATTR_ARTIFICIAL SG_ATTR_CONST SG_INLINE
unsigned
sg_rotr_32(unsigned x, unsigned k)
{
    return (x >> k) | (x << (32 - k));
}

#endif

/**
 * @brief Round a 32-bit integer up to the next power of two.
 *
 * Returns zero on overflow.
 */
SG_ATTR_ARTIFICIAL SG_ATTR_CONST SG_INLINE
unsigned
sg_round_up_pow2_32(unsigned x)
{
    x -= 1;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    return x + 1;
}

/**
 * @brief Round a @c size_t up to the next power of two.
 *
 * Returns zero on overflow.
 */
SG_ATTR_ARTIFICIAL SG_ATTR_CONST SG_INLINE
size_t
sg_round_up_pow2(size_t x)
{
    x -= 1;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    /* Any decent compiler will optimize this to x >> 32 or a nop, so
       this will work correctly on both 32-bit and 64-bit systems and
       it won't generate a warning for either.  */
    x |= (x >> 16) >> 16;
    return x + 1;
}

/**
 * @brief Round a @c size_t up to the next multiple of the largest
 * scalar data type's alignment.
 */
SG_ATTR_ARTIFICIAL SG_ATTR_CONST SG_INLINE
size_t
sg_align(size_t x)
{
    return (x + 7) & ~(size_t) 7;
}

/**
 * @brief Round a @c size_t up to the next multiple of the vector data
 * type's alignment.
 */
SG_ATTR_ARTIFICIAL SG_ATTR_CONST SG_INLINE
size_t
sg_align_vec(size_t x)
{
    return (x + 15) & ~(size_t) 15;
}

#ifdef __cplusplus
}
#endif
#endif
