/* Copyright 2011-2013 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#ifndef SG_STRBUF_H
#define SG_STRBUF_H
#include <assert.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include "sg/defs.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file strbuf.h
 *
 * @brief String buffers
 *
 * A string buffer is a buffer for constructing strings.  Writing to
 * the buffer automatically extends the buffer as necessary.  There is
 * also always one extra byte in the underlying memory containing NUL,
 * this is not counted in the length.  The extra byte is just for
 * convenience, so the data in the buffer can be used with functions
 * that expect a traditional C string.
 *
 * This is inspired by the strbuf API in Git.
 */

/**
 * @brief String buffer
 */
struct sg_strbuf {
    /**
     * @brief The start of the buffer.
     */
    char *s;

    /**
     * @brief The current buffer position.
     *
     * This always points to a NUL byte, and new data is written to
     * this location.
     */
    char *p;

    /**
     * @brief The end of the buffer.
     *
     * If the buffer is full, this points to the NUL byte at the end
     * of the string.  The number of bytes allocated is equal to the
     * difference between the start and the end, plus one.
     */
    char *e;
};

/**
 * @brief Initialize an empty string buffer.
 */
SG_ATTR_NONNULL((1))
void
sg_strbuf_init(struct sg_strbuf *b, size_t initsz);

/**
 * @brief Destroy a string buffer and free associated memory.
 *
 * The strbuf is reset to a zero-length string.
 */
SG_ATTR_NONNULL((1))
void
sg_strbuf_destroy(struct sg_strbuf *b);

/**
 * @brief Initialize a string buffer with preallocated data.
 *
 * Initialize a buffer with the given string which is in a region of
 * memory allocated with malloc which has the given length.  The
 * length must be at least one byte longer than the string, for the
 * NUL byte.
 */
SG_ATTR_NONNULL((1, 2))
void
sg_strbuf_attach(struct sg_strbuf *b, char *str, size_t len);

/**
 * @brief Get the string in the buffer, destroying the buffer.
 *
 * The result must be later freed with free().  The strbuf is reset to
 * a zero-length string.
 */
SG_ATTR_NONNULL((1))
char *
sg_strbuf_detach(struct sg_strbuf *b);

/**
 * @brief Get the amount of unused space remaining in the buffer.
 */
SG_ATTR_NONNULL((1)) SG_INLINE
size_t
sg_strbuf_avail(struct sg_strbuf *b)
{
    return b->e - b->p;
}

/**
 * @brief Reserve space in a string buffer.
 *
 * Allocate enough space in the buffer so the string can be expanded
 * by the given amount without additional allocations.
 */
SG_ATTR_NONNULL((1))
void
sg_strbuf_reserve(struct sg_strbuf *b, size_t len);

/**
 * @brief Shrink the buffer to match the current contents.
 */
SG_ATTR_NONNULL((1))
void
sg_strbuf_compact(struct sg_strbuf *b);

/**
 * @brief Set the string to the empty string, but don't free memory.
 */
SG_ATTR_NONNULL((1)) SG_INLINE
void
sg_strbuf_clear(struct sg_strbuf *b)
{
    b->p = b->s;
    *b->p = '\0';
}

/**
 * @brief Compare two strings.
 *
 * This is equivalent to calling strcmp() on the contained strings,
 * except NUL bytes are handled correctly.
 */
SG_ATTR_NONNULL((1, 2))
int
sg_strbuf_cmp(struct sg_strbuf *x, struct sg_strbuf *y);

/**
 * @brief Get the length of the string in the buffer.
 */
SG_ATTR_NONNULL((1)) SG_INLINE
size_t
sg_strbuf_len(struct sg_strbuf *b)
{
    return b->p - b->s;
}

/**
 * @brief Set the length of the string in the buffer.
 *
 * This must not be longer than the current length.
 */
SG_ATTR_NONNULL((1)) SG_INLINE
void
sg_strbuf_setlen(struct sg_strbuf *b, size_t len)
{
    size_t l = b->p - b->s;
    assert(len <= l);
    b->p = b->s + len;
    *b->p = '\0';
}

/**
 * @brief Set the length of the string in the buffer.
 *
 * This will expand the string as necessary, and assumes that you have
 * already reserved the space and written the data.
 */
SG_ATTR_NONNULL((1)) SG_INLINE
void
sg_strbuf_forcelen(struct sg_strbuf *b, size_t len)
{
    size_t l = b->e - b->s;
    assert(len <= l);
    b->p = b->s + len;
    *b->p = '\0';
}

/**
 * @brief Write bytes to the buffer.
 */
SG_ATTR_NONNULL((1))
void
sg_strbuf_write(struct sg_strbuf *b, const char *data, size_t len);

/**
 * @brief Write a character to the buffer.
 */
SG_ATTR_NONNULL((1)) SG_INLINE
void
sg_strbuf_putc(struct sg_strbuf *b, int c)
{
    if (b->p == b->e)
        sg_strbuf_reserve(b, 1);
    *b->p++ = c;
    *b->p = '\0';
}

/**
 * @brief Write a string to the buffer.
 */
SG_ATTR_NONNULL((1, 2)) SG_INLINE
void
sg_strbuf_puts(struct sg_strbuf *b, const char *str)
{
    sg_strbuf_write(b, str, strlen(str));
}

/**
 * @brief Write a formatted string to the buffer.
 */
SG_ATTR_FORMAT(printf, 2, 3) SG_ATTR_NONNULL((1, 2))
void
sg_strbuf_printf(struct sg_strbuf *b, const char *fmt, ...);

/**
 * @brief Write a formatted string to the buffer.
 */
SG_ATTR_NONNULL((1, 2))
void
sg_strbuf_vprintf(struct sg_strbuf *b, const char *fmt, va_list ap);

#ifdef __cplusplus
}
#endif
#endif
