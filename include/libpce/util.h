/* Copyright 2012 Dietrich Epp <depp@zdome.net> */
#ifndef PCE_UTIL_H
#define PCE_UTIL_H
#ifdef __cplusplus
extern "C" {
#endif
#include <libpce/attribute.h>
#include <libpce/byteorder.h>
#include <stddef.h>

/**
 * @defgroup util Utility functions
 *
 * These utility functions are miscellaneous useful integer functions.
 * This includes byte swapping, rotations, and alignmment.
 *
 * @{
 */

#if defined(__clang__)
# if __has_builtin(__builtin_bswap32)
#  define PCE_SWAP32 __builtin_bswap32
# endif
# if __has_builtin(__builtin_bswap64)
#  define PCE_SWAP64 __builtin_bswap64
# endif
#elif defined(__GNUC__)
# if __GNUC__ > 4 || (__GNUC__ >= 4 && __GNUC_MINOR__ >= 3)
#  define PCE_SWAP32 __builtin_bswap32
#  define PCE_SWAP64 __builtin_bswap64
# endif
#elif defined(_MSC_VER)
# include <stdlib.h>
# define PCE_SWAP16 _byteswap_ushort
# define PCE_SWAP32 _byteswap_ulong
# define PCE_SWAP64 _byteswap_uint64
#endif

/**
 * Swap the byte order of a 16-bit integer between big and little
 * endian.
 */
PCE_ATTR_ARTIFICIAL PCE_ATTR_CONST PCE_INLINE
unsigned short
pce_swap16(unsigned short x);

/**
 * Swap the byte order of a 32-bit integer between big and little
 * endian.
 */
PCE_ATTR_ARTIFICIAL PCE_ATTR_CONST PCE_INLINE
unsigned
pce_swap32(unsigned x);

/**
 * Swap the byte order of a 64-bit integer between big and little
 * endian.
 */
PCE_ATTR_ARTIFICIAL PCE_ATTR_CONST PCE_INLINE
unsigned long long
pce_swap64(unsigned long long x);

#if defined(PCE_SWAP16)
PCE_ATTR_ARTIFICIAL PCE_ATTR_CONST PCE_INLINE
unsigned short
pce_swap16(unsigned short x)
{
    return PCE_SWAP16(x);
}
#elif defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
PCE_ATTR_ARTIFICIAL PCE_ATTR_CONST PCE_INLINE
unsigned short
pce_swap16(unsigned short x)
{
    __asm__("xchgb %b0,%h0" : "=Q"(x) : "0"(x));
    return x;
}
#elif defined(__GNUC__) && (defined(__powerpce__) || defined(__ppce__))
PCE_ATTR_ARTIFICIAL PCE_ATTR_CONST PCE_INLINE
unsigned short
pce_swap16(unsigned short x)
{
    unsigned tmp;
    __asm__("rlwimi %0,%2,8,16,23"
            : "=r"(tmp)
            : "0"(x >> 8), "r"(x));
    return (unsigned short) tmp;
}
#else
PCE_ATTR_ARTIFICIAL PCE_ATTR_CONST PCE_INLINE
unsigned short
pce_swap16(unsigned short x)
{
    return (unsigned short) ((x >> 8) | (x << 8));
}
#endif

#if defined(PCE_SWAP32)
PCE_ATTR_ARTIFICIAL PCE_ATTR_CONST PCE_INLINE
unsigned
pce_swap32(unsigned x)
{
    return PCE_SWAP32(x);
}
#elif defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
PCE_ATTR_ARTIFICIAL PCE_ATTR_CONST PCE_INLINE
unsigned
pce_swap32(unsigned x)
{
    __asm__("bswap %0" : "=r"(x) : "0"(x));
    return x;
}
#elif defined(__GNUC__) && (defined(__powerpce__) || defined(__ppce__))
PCE_ATTR_ARTIFICIAL PCE_ATTR_CONST PCE_INLINE
unsigned
pce_swap32(unsigned x)
{
    unsigned tmp;
    __asm__("rotlwi %0,%1,8" : "=r"(tmp) : "r"(x));
    __asm__("rlwimi %0,%2,24,0,7" : "=r"(tmp) : "0"(tmp), "r"(x));
    __asm__("rlwimi %0,%2,24,16,23" : "=r"(tmp) : "0"(tmp), "r"(x));
    return tmp;
}
#else
PCE_ATTR_ARTIFICIAL PCE_ATTR_CONST PCE_INLINE
unsigned
pce_swap32(unsigned x)
{
    return
        ((x & 0xff000000) >> 24) |
        ((x & 0x00ff0000) >> 8) |
        ((x & 0x0000ff00) << 8) |
        ((x & 0x000000ff) << 24);
}
#endif

#if defined(PCE_SWAP64)
PCE_ATTR_ARTIFICIAL PCE_ATTR_CONST PCE_INLINE
unsigned long long
pce_swap64(unsigned long long x)
{
    return pce_swap64_builtin(x);
}
#elif defined(__GNUC__) && defined(__x86_64__)
PCE_ATTR_ARTIFICIAL PCE_ATTR_CONST PCE_INLINE
unsigned long long
pce_swap64(unsigned long long x)
{
    __asm__("bswapq %0" : "=r"(x) : "0"(x));
}
#else
PCE_ATTR_ARTIFICIAL PCE_ATTR_CONST PCE_INLINE
unsigned long long
pce_swap64(unsigned long long x)
{
    return
        (unsigned long long) pce_swap32((unsigned) (x >> 32)) |
        ((unsigned long long) pce_swap32((unsigned) x) << 32);
}
#endif

/**
 * @def pce_swapbe16(x)
 *
 * Swap the byte order of a 16-bit integer between native and big
 * endian.
 *
 * @def pce_swapbe32(x)
 *
 * Swap the byte order of a 32-bit integer between native and big
 * endian.
 *
 * @def pce_swapbe64(x)
 *
 * Swap the byte order of a 64-bit integer between native and big
 * endian.
 *
 * @def pce_swaple16(x)
 *
 * Swap the byte order of a 16-bit integer between native and little
 * endian.
 *
 * @def pce_swaple32(x)
 *
 * Swap the byte order of a 32-bit integer between native and little
 * endian.
 *
 * @def pce_swaple64(x)
 *
 * Swap the byte order of a 64-bit integer between native and little
 * endian.
 */

#if PCE_BYTE_ORDER == PCE_LITTLE_ENDIAN
# define pce_swapbe16(x) pce_swap16(x)
# define pce_swapbe32(x) pce_swap32(x)
# define pce_swapbe64(x) pce_swap64(x)
# define pce_swaple16(x) (x)
# define pce_swaple32(x) (x)
# define pce_swaple64(x) (x)
#else
# define pce_swapbe16(x) (x)
# define pce_swapbe32(x) (x)
# define pce_swapbe64(x) (x)
# define pce_swaple16(x) pce_swap16(x)
# define pce_swaple32(x) pce_swap32(x)
# define pce_swaple64(x) pce_swap64(x)
#endif

/**
 * Rotate a 32-bit integer to the left.
 */
PCE_ATTR_ARTIFICIAL PCE_ATTR_CONST PCE_INLINE
unsigned
pce_rotl_32(unsigned x, unsigned k);

/**
 * Rotate a 32-bit integer to the right.
 */
PCE_ATTR_ARTIFICIAL PCE_ATTR_CONST PCE_INLINE
unsigned
pce_rotr_32(unsigned x, unsigned k);

#if defined(_MSC_VER)

#include <stdlib.h>

PCE_ATTR_ARTIFICIAL PCE_ATTR_CONST PCE_INLINE
unsigned
pce_rotl_32(unsigned x, unsigned k)
{
    return _rotl(x, k);
}

PCE_ATTR_ARTIFICIAL PCE_ATTR_CONST PCE_INLINE
unsigned
pce_rotr_32(unsigned x, unsigned k)
{
    return _rotr(x, k);
}

#else

PCE_ATTR_ARTIFICIAL PCE_ATTR_CONST PCE_INLINE
unsigned
pce_rotl_32(unsigned x, unsigned k)
{
    return (x << k) | (x >> (32 - k));
}

PCE_ATTR_ARTIFICIAL PCE_ATTR_CONST PCE_INLINE
unsigned
pce_rotr_32(unsigned x, unsigned k)
{
    return (x >> k) | (x << (32 - k));
}

#endif

/**
 * Round a 32-bit integer up to the next power of two.  Returns zero
 * on overflow.
 */
PCE_ATTR_ARTIFICIAL PCE_ATTR_CONST PCE_INLINE
unsigned
pce_round_up_pow2_32(unsigned x)
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
 * Round a @c size_t up to the next power of two.  Returns zero on
 * overflow.
 */
PCE_ATTR_ARTIFICIAL PCE_ATTR_CONST PCE_INLINE
size_t
pce_round_up_pow2(size_t x)
{
    x -= 1;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    if (sizeof(x) == 8)
        x |= x >> 32;
    return x + 1;
}

/**
 * Round a @c size_t up to the next multiple of the largest scalar
 * data type's alignment.
 */
PCE_ATTR_ARTIFICIAL PCE_ATTR_CONST PCE_INLINE
size_t
pce_align(size_t x)
{
    return (x + 7) & ~(size_t) 7;
}

/**
 * Round a @c size_t up to the next multiple of the vector data type's
 * alignment.
 */
PCE_ATTR_ARTIFICIAL PCE_ATTR_CONST PCE_INLINE
size_t
pce_align_vec(size_t x)
{
    return (x + 15) & ~(size_t) 15;
}

/** @} */

#ifdef __cplusplus
}
#endif
#endif
