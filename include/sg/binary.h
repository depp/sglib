/* Copyright 2009-2013 Dietrich Epp.
   This file is part of LibPCE.  LibPCE is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#ifndef PCE_BINARY_H
#define PCE_BINARY_H
#include <stdint.h>
#include "sg/attribute.h"

/**
 * @file binary.h
 *
 * @brief Binary serialization / deserialization
 *
 * Functions for serializing / deserializing binary data.  Each
 * function is named using the following parts:
 *
 * - read/write
 * - b/l: big/little endian
 * - s/u: signed/unsigned integer
 */

/** @brief Read an unsigned 8 bit integer.  */
PCE_INLINE unsigned char
pce_read_u8(void const *ptr)
{
    return *(unsigned char const *) ptr;
}

/** @brief Read an signed 8 bit integer.  */
PCE_INLINE signed char
pce_read_s8(void const *ptr)
{
    return *(signed char const *) ptr;
}

/** @brief Read a big endian unsigned 16 bit integer.  */
PCE_INLINE unsigned short
pce_read_bu16(void const *ptr)
{
    unsigned char const *p = (const unsigned char *) ptr;
    unsigned short r = ((unsigned short) p[0] << 8) | (unsigned short)p[1];
    return r;
}

/** @brief Read a little endian unsigned 16 bit integer.  */
PCE_INLINE unsigned short
pce_read_lu16(void const *ptr)
{
    unsigned char const *p = (const unsigned char *) ptr;
    unsigned short r = ((unsigned short) p[1] << 8) | (unsigned short)p[0];
    return r;
}

/** @brief Read a big endian signed 16 bit integer.  */
PCE_INLINE short
pce_read_bs16(void const *ptr)
{
    return (short) pce_read_bu16(ptr);
}

/** @brief Read a little endian signed 16 bit integer.  */
PCE_INLINE short
pce_read_ls16(void const *ptr)
{
    return (short) pce_read_lu16(ptr);
}

/** @brief Read big endian unsigned 32 bit integer.  */
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

/** @brief Read a little endian unsigned 32 bit integer.  */
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

/** @brief Read a big endian signed 32 bit integer.  */
PCE_INLINE int
pce_read_bs32(void const *ptr)
{
    return (int) pce_read_bu32(ptr);
}

/** @brief Read a little endian signed 32 bit integer.  */
PCE_INLINE int
pce_read_ls32(void const *ptr)
{
    return (int) pce_read_lu32(ptr);
}

/** @brief Read big endian unsigned 64 bit integer.  */
PCE_INLINE unsigned long long
pce_read_bu64(void const *ptr)
{
    unsigned char const *p = (const unsigned char *) ptr;
    return ((unsigned long long) pce_read_bu32(p) << 32) |
        pce_read_bu32(p + 4);
}

/** @brief Read big endian unsigned 64 bit integer.  */
PCE_INLINE unsigned long long
pce_read_lu64(void const *ptr)
{
    unsigned char const *p = (const unsigned char *) ptr;
    return ((unsigned long long) pce_read_lu32(p + 4) << 32) |
        pce_read_lu32(p);
}

/** @brief Read a big endian signed 64 bit integer.  */
PCE_INLINE long long
pce_read_bs64(void const *ptr)
{
    return (long long) pce_read_bu64(ptr);
}

/** @brief Read a little endian signed 64 bit integer.  */
PCE_INLINE long long
pce_read_ls64(void const *ptr)
{
    return (long long) pce_read_lu64(ptr);
}

/** @brief Write an unsigned 8 bit integer.  */
PCE_INLINE void
pce_write_u8(void *ptr, unsigned char v)
{
    *(unsigned char *) ptr = v;
}

/** @brief Write an unsigned 8 bit integer.  */
PCE_INLINE void
pce_write_s8(void *ptr, signed char v)
{
    *(signed char *) ptr = v;
}

/** @brief Write a big endian unsigned 16 bit integer.  */
PCE_INLINE void
pce_write_bu16(void *ptr, unsigned short v)
{
    unsigned char *p = (unsigned char *) ptr;
    p[0] = (unsigned char) (v >> 8);
    p[1] = (unsigned char) v;
}

/** @brief Write a little endian unsigned 16 bit integer.  */
PCE_INLINE void
pce_write_lu16(void *ptr, unsigned short v)
{
    unsigned char *p = (unsigned char *) ptr;
    p[0] = (unsigned char) v;
    p[1] = (unsigned char) (v >> 8);
}

/** @brief Write a big endian signed 16 bit integer.  */
PCE_INLINE void
pce_write_bs16(void *ptr, short v)
{
    pce_write_bu16(ptr, v);
}

/** @brief Write a little endian signed 16 bit integer.  */
PCE_INLINE void
pce_write_ls16(void *ptr, short v)
{
    pce_write_lu16(ptr, v);
}

/** @brief Write a big endian unsigned 32 bit integer.  */
PCE_INLINE void
pce_write_bu32(void *ptr, unsigned v)
{
    unsigned char *p = (unsigned char *) ptr;
    p[0] = v >> 24;
    p[1] = v >> 16;
    p[2] = v >> 8;
    p[3] = v;
}

/** @brief Write a little endian unsigned 32 bit integer.  */
PCE_INLINE void
pce_write_lu32(void *ptr, unsigned v)
{
    unsigned char *p = (unsigned char *) ptr;
    p[0] = v;
    p[1] = v >> 8;
    p[2] = v >> 16;
    p[3] = v >> 24;
}

/** @brief Write a big endian signed 32 bit integer.  */
PCE_INLINE void
pce_write_bs32(void *ptr, int v)
{
    pce_write_bu32(ptr, v);
}

/** @brief Write a little endian signed 32 bit integer.  */
PCE_INLINE void
pce_write_ls32(void *ptr, int v)
{
    pce_write_lu32(ptr, v);
}

/** @brief Write a big endian unsigned 64 bit integer.  */
PCE_INLINE void
pce_write_bu64(void *ptr, unsigned long long v)
{
    unsigned char *p = (unsigned char *) ptr;
    pce_write_bu32(p, (unsigned) (v >> 32));
    pce_write_bu32(p + 4, (unsigned) v);
}

/** @brief Write a little endian unsigned 64 bit integer.  */
PCE_INLINE void
pce_write_lu64(void *ptr, unsigned long long v)
{
    unsigned char *p = (unsigned char *) ptr;
    pce_write_lu32(p, (unsigned) v);
    pce_write_lu32(p + 4, (unsigned) (v >> 32));
}

/** @brief Write a big endian signed 64 bit integer.  */
PCE_INLINE void
pce_write_bs64(void *ptr, long long v)
{
    pce_write_bu64(ptr, v);
}

/** @brief Write a little endian signed 64 bit integer.  */
PCE_INLINE void
pce_write_ls64(void *ptr, long long v)
{
    pce_write_lu64(ptr, v);
}

#endif
