/* Copyright 2012 Dietrich Epp <depp@zdome.net> */
/*
  Portable byte swapping functions.

  pce_swap16: swap a 16-bit value between big and little endian
  pce_swap32: swap a 32-bit value between big and little endian
  pce_swap64: swap a 64-bit value between big and little endian

  pce_swapbe16: swap a 16-bit value between native and big endian
  pce_swapbe32: swap a 32-bit value between native and big endian
  pce_swapbe64: swap a 64-bit value between native and big endian

  pce_swaple16: swap a 16-bit value between native and little endian
  pce_swaple32: swap a 32-bit value between native and little endian
  pce_swaple64: swap a 64-bit value between native and little endian
*/
#ifndef PCE_BYTESWAP_H
#define PCE_BYTESWAP_H
#include <libpce/attribute.h>
#include <libpce/byteorder.h>
#ifdef __cplusplus
extern "C" {
#endif

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

#if defined(PCE_SWAP16)
PCE_ATTR_CONST
static __inline unsigned short pce_swap16(unsigned short x)
{
    return PCE_SWAP16(x);
}
#elif defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
PCE_ATTR_CONST
static __inline unsigned short pce_swap16(unsigned short x)
{
    __asm__("xchgb %b0,%h0" : "=Q"(x) : "0"(x));
    return x;
}
#elif defined(__GNUC__) && (defined(__powerpce__) || defined(__ppce__))
PCE_ATTR_CONST
static __inline unsigned short pce_swap16(unsigned short x)
{
    unsigned tmp;
    __asm__("rlwimi %0,%2,8,16,23"
            : "=r"(tmp)
            : "0"(x >> 8), "r"(x));
    return (unsigned short) tmp;
}
#else
PCE_ATTR_CONST
static __inline unsigned short pce_swap16(unsigned short x)
{
    return (unsigned short) ((x >> 8) | (x << 8));
}
#endif

#if defined(PCE_SWAP32)
PCE_ATTR_CONST
static __inline unsigned pce_swap32(unsigned x)
{
    return PCE_SWAP32(x);
}
#elif defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
PCE_ATTR_CONST
static __inline unsigned pce_swap32(unsigned x)
{
    __asm__("bswap %0" : "=r"(x) : "0"(x));
    return x;
}
#elif defined(__GNUC__) && (defined(__powerpce__) || defined(__ppce__))
PCE_ATTR_CONST
static __inline unsigned pce_swap32(unsigned x)
{
    unsigned tmp;
    __asm__("rotlwi %0,%1,8" : "=r"(tmp) : "r"(x));
    __asm__("rlwimi %0,%2,24,0,7" : "=r"(tmp) : "0"(tmp), "r"(x));
    __asm__("rlwimi %0,%2,24,16,23" : "=r"(tmp) : "0"(tmp), "r"(x));
    return tmp;
}
#else
PCE_ATTR_CONST
static __inline unsigned pce_swap32(unsigned x)
    return
        ((x & 0xff000000) >> 24) |
        ((x & 0x00ff0000) >> 8) |
        ((x & 0x0000ff00) << 8) |
        ((x & 0x000000ff) << 24);
}
#endif

#if defined(PCE_SWAP64)
PCE_ATTR_CONST
static __inline unsigned long long pce_swap64(unsigned long long x)
{
    return pce_swap64_builtin(x);
}
#elif defined(__GNUC__) && defined(__x86_64__)
PCE_ATTR_CONST
static __inline unsigned long long pce_swap64(unsigned long long x)
{
    __asm__("bswapq %0" : "=r"(x) : "0"(x));
}
#else
PCE_ATTR_CONST
static __inline unsigned long long pce_swap64(unsigned long long x)
    return
        (unsigned long long) pce_swap32((unsigned) (x >> 32)) |
        ((unsigned long long) pce_swap32((unsigned) x) << 32);
}
#endif

#if PCE_BYTE_ORDER == PCE_LITTLE_ENDIAN
# define pce_swapbe16(x) pce_swap16(x)
# define pce_swapbe32(x) pce_swap32(x)
# define pce_swapbe64(x) pce_swap64(x)
# define pce_swaple16(x) (x)
# define pce_swaple32(x) (x)
# define pce_swaple64(x) (x)
#elif PCE_BYTE_ORDER == PCE_BIG_ENDIAN
# define pce_swapbe16(x) (x)
# define pce_swapbe32(x) (x)
# define pce_swapbe64(x) (x)
# define pce_swaple16(x) pce_swap16(x)
# define pce_swaple32(x) pce_swap32(x)
# define pce_swaple64(x) pce_swap64(x)
#endif

#ifdef __cplusplus
}
#endif
#endif
