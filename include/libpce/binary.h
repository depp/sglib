/* Copyright 2009-2012 Dietrich Epp <depp@zdome.net> */
#ifndef PCE_BINARY_H
#define PCE_BINARY_H
#include <stdint.h>
#include "libpce/attribute.h"

/**
 * @defgroup binary Binary serialization / deserialization
 *
 * Functions for serializing / deserializing binary data.  Each
 * function is named using the following parts:
 *
 * - read/write
 * - b/l: big/little endian
 * - s/u: signed/unsigned integer
 *
 * @{
 */

/* Read an unsigned 8 bit integer.  */
PCE_INLINE unsigned char
pce_read_u8(void const *ptr)
{
    return *(unsigned char const *) ptr;
}

/* Read an signed 8 bit integer.  */
PCE_INLINE signed char
pce_read_s8(void const *ptr)
{
    return *(signed char const *) ptr;
}

/* Read a big endian unsigned 16 bit integer.  */
PCE_INLINE unsigned short
pce_read_bu16(void const *ptr)
{
    unsigned char const *p = (const unsigned char *) ptr;
    unsigned short r = ((unsigned short) p[0] << 8) | (unsigned short)p[1];
    return r;
}

/* Read a little endian unsigned 16 bit integer.  */
PCE_INLINE unsigned short
pce_read_lu16(void const *ptr)
{
    unsigned char const *p = (const unsigned char *) ptr;
    unsigned short r = ((unsigned short) p[1] << 8) | (unsigned short)p[0];
    return r;
}

/* Read a big endian signed 16 bit integer.  */
PCE_INLINE short
pce_read_bs16(void const *ptr)
{
    return (short) pce_read_bu16(ptr);
}

/* Read a little endian signed 16 bit integer.  */
PCE_INLINE short
pce_read_ls16(void const *ptr)
{
    return (short) pce_read_lu16(ptr);
}

/* Read big endian unsigned 32 bit integer.  */
PCE_INLINE unsigned
pce_read_bu32(void const *ptr)
{
    unsigned char const *p = (const unsigned char *) ptr;
    unsigned r = ((unsigned) p[0] << 24)
        | ((unsigned) p[1] << 16)
        | ((unsigned) p[2] << 8)
        | (unsigned) p[3];
    return r;
}

/* Read a little endian unsigned 32 bit integer.  */
PCE_INLINE unsigned
pce_read_lu32(void const *ptr)
{
    unsigned char const *p = (const unsigned char *) ptr;
    unsigned r = ((unsigned) p[3] << 24)
        | ((unsigned) p[2] << 16)
        | ((unsigned) p[1] << 8)
        | (unsigned) p[0];
    return r;
}

/* Read a big endian signed 32 bit integer.  */
PCE_INLINE int
pce_read_bs32(void const *ptr)
{
    return (int) pce_read_bu32(ptr);
}

/* Read a little endian signed 32 bit integer.  */
PCE_INLINE int
pce_read_ls32(void const *ptr)
{
    return (int) pce_read_lu32(ptr);
}

/* Read big endian unsigned 64 bit integer.  */
PCE_INLINE unsigned long long
pce_read_bu64(void const *ptr)
{
    unsigned char const *p = (const unsigned char *) ptr;
    return ((unsigned long long) pce_read_bu32(p) << 32) | pce_read_bu32(p + 4);
}

/* Read big endian unsigned 64 bit integer.  */
PCE_INLINE unsigned long long
pce_read_lu64(void const *ptr)
{
    unsigned char const *p = (const unsigned char *) ptr;
    return ((unsigned long long) pce_read_lu32(p + 4) << 32) | pce_read_lu32(p);
}

/* Read a big endian signed 64 bit integer.  */
PCE_INLINE long long
pce_read_bs64(void const *ptr)
{
    return (long long) pce_read_bu64(ptr);
}

/* Read a little endian signed 64 bit integer.  */
PCE_INLINE long long
pce_read_ls64(void const *ptr)
{
    return (long long) pce_read_lu64(ptr);
}

/* Write an unsigned 8 bit integer.  */
PCE_INLINE void
pce_write_u8(void *ptr, unsigned char v)
{
    *(unsigned char *) ptr = v;
}

/* Write an unsigned 8 bit integer.  */
PCE_INLINE void
pce_write_s8(void *ptr, signed char v)
{
    *(signed char *) ptr = v;
}

/* Write a big endian unsigned 16 bit integer.  */
PCE_INLINE void
pce_write_bu16(void *ptr, unsigned short v)
{
    unsigned char *p = (unsigned char *) ptr;
    p[0] = v >> 8;
    p[1] = v;
}

/* Write a little endian unsigned 16 bit integer.  */
PCE_INLINE void
pce_write_lu16(void *ptr, unsigned short v)
{
    unsigned char *p = (unsigned char *) ptr;
    p[0] = v;
    p[1] = v >> 8;
}

/* Write a big endian signed 16 bit integer.  */
PCE_INLINE void
pce_write_bs16(void *ptr, short v)
{
    pce_write_bu16(ptr, v);
}

/* Write a little endian signed 16 bit integer.  */
PCE_INLINE void
pce_write_ls16(void *ptr, short v)
{
    pce_write_lu16(ptr, v);
}

/* Write a big endian unsigned 32 bit integer.  */
PCE_INLINE void
pce_write_bu32(void *ptr, unsigned v)
{
    unsigned char *p = (unsigned char *) ptr;
    p[0] = v >> 24;
    p[1] = v >> 16;
    p[2] = v >> 8;
    p[3] = v;
}

/* Write a little endian unsigned 32 bit integer.  */
PCE_INLINE void
pce_write_lu32(void *ptr, unsigned v)
{
    unsigned char *p = (unsigned char *) ptr;
    p[0] = v;
    p[1] = v >> 8;
    p[2] = v >> 16;
    p[3] = v >> 24;
}

/* Write a big endian signed 32 bit integer.  */
PCE_INLINE void
pce_write_bs32(void *ptr, int v)
{
    pce_write_bu32(ptr, v);
}

/* Write a little endian signed 32 bit integer.  */
PCE_INLINE void
pce_write_ls32(void *ptr, int v)
{
    pce_write_lu32(ptr, v);
}

/* Write a big endian unsigned 64 bit integer.  */
PCE_INLINE void
pce_write_bu64(void *ptr, unsigned long long v)
{
    unsigned char *p = (unsigned char *) ptr;
    pce_write_bu32(p, v >> 32);
    pce_write_bu32(p + 4, v);
}

/* Write a little endian unsigned 64 bit integer.  */
PCE_INLINE void
pce_write_lu64(void *ptr, unsigned long long v)
{
    unsigned char *p = (unsigned char *) ptr;
    pce_write_lu32(p, v);
    pce_write_lu32(p + 4, v >> 32);
}

/* Write a big endian signed 64 bit integer.  */
PCE_INLINE void
pce_write_bs64(void *ptr, long long v)
{
    pce_write_bu64(ptr, v);
}

/* Write a little endian signed 64 bit integer.  */
PCE_INLINE void
pce_write_ls64(void *ptr, long long v)
{
    pce_write_lu64(ptr, v);
}

/** @} */

#endif
