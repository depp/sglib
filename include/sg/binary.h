/* Copyright 2009-2013 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#ifndef SG_BINARY_H
#define SG_BINARY_H
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include "sg/defs.h"

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
SG_INLINE unsigned char
sg_read_u8(void const *ptr)
{
    return *(unsigned char const *) ptr;
}

/** @brief Read an signed 8 bit integer.  */
SG_INLINE signed char
sg_read_s8(void const *ptr)
{
    return *(signed char const *) ptr;
}

/** @brief Read a big endian unsigned 16 bit integer.  */
SG_INLINE unsigned short
sg_read_bu16(void const *ptr)
{
    unsigned char const *p = (const unsigned char *) ptr;
    unsigned short r = ((unsigned short) p[0] << 8) | (unsigned short)p[1];
    return r;
}

/** @brief Read a little endian unsigned 16 bit integer.  */
SG_INLINE unsigned short
sg_read_lu16(void const *ptr)
{
    unsigned char const *p = (const unsigned char *) ptr;
    unsigned short r = ((unsigned short) p[1] << 8) | (unsigned short)p[0];
    return r;
}

/** @brief Read a big endian signed 16 bit integer.  */
SG_INLINE short
sg_read_bs16(void const *ptr)
{
    return (short) sg_read_bu16(ptr);
}

/** @brief Read a little endian signed 16 bit integer.  */
SG_INLINE short
sg_read_ls16(void const *ptr)
{
    return (short) sg_read_lu16(ptr);
}

/** @brief Read big endian unsigned 32 bit integer.  */
SG_INLINE unsigned
sg_read_bu32(void const *ptr)
{
    unsigned char const *p = (const unsigned char *) ptr;
    unsigned r = ((unsigned) p[0] << 24)
        | ((unsigned) p[1] << 16)
        | ((unsigned) p[2] << 8)
        | (unsigned) p[3];
    return r;
}

/** @brief Read a little endian unsigned 32 bit integer.  */
SG_INLINE unsigned
sg_read_lu32(void const *ptr)
{
    unsigned char const *p = (const unsigned char *) ptr;
    unsigned r = ((unsigned) p[3] << 24)
        | ((unsigned) p[2] << 16)
        | ((unsigned) p[1] << 8)
        | (unsigned) p[0];
    return r;
}

/** @brief Read a big endian signed 32 bit integer.  */
SG_INLINE int
sg_read_bs32(void const *ptr)
{
    return (int) sg_read_bu32(ptr);
}

/** @brief Read a little endian signed 32 bit integer.  */
SG_INLINE int
sg_read_ls32(void const *ptr)
{
    return (int) sg_read_lu32(ptr);
}

/** @brief Read big endian unsigned 64 bit integer.  */
SG_INLINE unsigned long long
sg_read_bu64(void const *ptr)
{
    unsigned char const *p = (const unsigned char *) ptr;
    return ((unsigned long long) sg_read_bu32(p) << 32) |
        sg_read_bu32(p + 4);
}

/** @brief Read big endian unsigned 64 bit integer.  */
SG_INLINE unsigned long long
sg_read_lu64(void const *ptr)
{
    unsigned char const *p = (const unsigned char *) ptr;
    return ((unsigned long long) sg_read_lu32(p + 4) << 32) |
        sg_read_lu32(p);
}

/** @brief Read a big endian signed 64 bit integer.  */
SG_INLINE long long
sg_read_bs64(void const *ptr)
{
    return (long long) sg_read_bu64(ptr);
}

/** @brief Read a little endian signed 64 bit integer.  */
SG_INLINE long long
sg_read_ls64(void const *ptr)
{
    return (long long) sg_read_lu64(ptr);
}

/** @brief Write an unsigned 8 bit integer.  */
SG_INLINE void
sg_write_u8(void *ptr, unsigned char v)
{
    *(unsigned char *) ptr = v;
}

/** @brief Write an unsigned 8 bit integer.  */
SG_INLINE void
sg_write_s8(void *ptr, signed char v)
{
    *(signed char *) ptr = v;
}

/** @brief Write a big endian unsigned 16 bit integer.  */
SG_INLINE void
sg_write_bu16(void *ptr, unsigned short v)
{
    unsigned char *p = (unsigned char *) ptr;
    p[0] = (unsigned char) (v >> 8);
    p[1] = (unsigned char) v;
}

/** @brief Write a little endian unsigned 16 bit integer.  */
SG_INLINE void
sg_write_lu16(void *ptr, unsigned short v)
{
    unsigned char *p = (unsigned char *) ptr;
    p[0] = (unsigned char) v;
    p[1] = (unsigned char) (v >> 8);
}

/** @brief Write a big endian signed 16 bit integer.  */
SG_INLINE void
sg_write_bs16(void *ptr, short v)
{
    sg_write_bu16(ptr, v);
}

/** @brief Write a little endian signed 16 bit integer.  */
SG_INLINE void
sg_write_ls16(void *ptr, short v)
{
    sg_write_lu16(ptr, v);
}

/** @brief Write a big endian unsigned 32 bit integer.  */
SG_INLINE void
sg_write_bu32(void *ptr, unsigned v)
{
    unsigned char *p = (unsigned char *) ptr;
    p[0] = v >> 24;
    p[1] = v >> 16;
    p[2] = v >> 8;
    p[3] = v;
}

/** @brief Write a little endian unsigned 32 bit integer.  */
SG_INLINE void
sg_write_lu32(void *ptr, unsigned v)
{
    unsigned char *p = (unsigned char *) ptr;
    p[0] = v;
    p[1] = v >> 8;
    p[2] = v >> 16;
    p[3] = v >> 24;
}

/** @brief Write a big endian signed 32 bit integer.  */
SG_INLINE void
sg_write_bs32(void *ptr, int v)
{
    sg_write_bu32(ptr, v);
}

/** @brief Write a little endian signed 32 bit integer.  */
SG_INLINE void
sg_write_ls32(void *ptr, int v)
{
    sg_write_lu32(ptr, v);
}

/** @brief Write a big endian unsigned 64 bit integer.  */
SG_INLINE void
sg_write_bu64(void *ptr, unsigned long long v)
{
    unsigned char *p = (unsigned char *) ptr;
    sg_write_bu32(p, (unsigned) (v >> 32));
    sg_write_bu32(p + 4, (unsigned) v);
}

/** @brief Write a little endian unsigned 64 bit integer.  */
SG_INLINE void
sg_write_lu64(void *ptr, unsigned long long v)
{
    unsigned char *p = (unsigned char *) ptr;
    sg_write_lu32(p, (unsigned) v);
    sg_write_lu32(p + 4, (unsigned) (v >> 32));
}

/** @brief Write a big endian signed 64 bit integer.  */
SG_INLINE void
sg_write_bs64(void *ptr, long long v)
{
    sg_write_bu64(ptr, v);
}

/** @brief Write a little endian signed 64 bit integer.  */
SG_INLINE void
sg_write_ls64(void *ptr, long long v)
{
    sg_write_lu64(ptr, v);
}

#ifdef __cplusplus
}
#endif
#endif
