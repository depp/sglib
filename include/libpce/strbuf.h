/* Copyright 2011-2012 Dietrich Epp <depp@zdome.net> */
#ifndef PCE_STRBUF_H
#define PCE_STRBUF_H
#include <assert.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <libpce/attribute.h>
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup strbuf String buffers
 *
 * A string buffer is a buffer for constructing strings.  Writing to
 * the buffer automatically extends the buffer as necessary.  There is
 * also always one extra byte in the underlying memory containing NUL,
 * this is not counted in the length.  The extra byte is just for
 * convenience, so the data in the buffer can be used with functions
 * that expect a traditional C string.
 *
 * This is inspired by the strbuf API in Git.
 *
 * @{
 */

/**
 * String buffer
 */
struct pce_strbuf {
    /**
     * The start of the buffer.
     */
    char *s;

    /**
     * The current buffer position.  This always points to a NUL byte,
     * and new data is written to this location.
     */
    char *p;

    /**
     * The end of the buffer.  If the buffer is full, this points to
     * the NUL byte at the end of the string.  The number of bytes
     * allocated is equal to the difference between the start and the
     * end, plus one.
     */
    char *e;
};

/**
 * Initialize an empty string buffer.
 */
PCE_ATTR_NONNULL((1))
void
pce_strbuf_init(struct pce_strbuf *b, size_t initsz);

/**
 * Destroy a string buffer and free associated memory.  The strbuf is
 * reset to a zero-length string.
 */
PCE_ATTR_NONNULL((1))
void
pce_strbuf_destroy(struct pce_strbuf *b);

/**
 * Initialize a buffer with the given string which is in a region of
 * memory allocated with malloc which has the given length.  The
 * length must be at least one byte longer than the string, for the
 * NUL byte.
 */
PCE_ATTR_NONNULL((1, 2))
void
pce_strbuf_attach(struct pce_strbuf *b, char *str, size_t len);

/**
 * Return the string in the buffer, which must be later freed with
 * free().  The strbuf is reset to a zero-length string.
 */
PCE_ATTR_NONNULL((1))
char *
pce_strbuf_detach(struct pce_strbuf *b);

/**
 * Get the amount of unused space remaining in the buffer.
 */
PCE_ATTR_NONNULL((1)) PCE_INLINE
size_t
pce_strbuf_avail(struct pce_strbuf *b)
{
    return b->e - b->p;
}

/**
 * Allocate enough space in the buffer so the string can be expanded
 * by the given amount without additional allocations.
 */
PCE_ATTR_NONNULL((1))
void
pce_strbuf_reserve(struct pce_strbuf *b, size_t len);

/**
 * Shrink the buffer to match the current contents.
 */
PCE_ATTR_NONNULL((1))
void
pce_strbuf_compact(struct pce_strbuf *b);

/**
 * Set the string to the empty string, but don't free memory.
 */
PCE_ATTR_NONNULL((1)) PCE_INLINE
void
pce_strbuf_clear(struct pce_strbuf *b)
{
    b->p = b->s;
    *b->p = '\0';
}

/**
 * Compare two strings.  This is equivalent to calling strcmp() on the
 * contained strings, except NUL bytes are handled correctly.
 */
PCE_ATTR_NONNULL((1, 2))
int
pce_strbuf_cmp(struct pce_strbuf *x, struct pce_strbuf *y);

/**
 * Get the length of the string in the buffer.
 */
PCE_ATTR_NONNULL((1)) PCE_INLINE
size_t
pce_strbuf_len(struct pce_strbuf *b)
{
    return b->p - b->s;
}

/**
 * Set the length of the string in the buffer.  This must not be
 * longer than the current length.
 */
PCE_ATTR_NONNULL((1)) PCE_INLINE
void
pce_strbuf_setlen(struct pce_strbuf *b, size_t len)
{
    size_t l = b->p - b->s;
    assert(len <= l);
    b->p = b->s + len;
    *b->p = '\0';
}

/**
 * Set the length of the string in the buffer.  This will expand the
 * string as necessary, and assumes that you have already reserved the
 * space and written the data.
 */
PCE_ATTR_NONNULL((1)) PCE_INLINE
void
pce_strbuf_forcelen(struct pce_strbuf *b, size_t len)
{
    size_t l = b->e - b->s;
    assert(len <= l);
    b->p = b->s + len;
    *b->p = '\0';
}

/**
 * Write bytes to the buffer.
 */
PCE_ATTR_NONNULL((1))
void
pce_strbuf_write(struct pce_strbuf *b, const char *data, size_t len);

/**
 * Write a character to the buffer.
 */
PCE_ATTR_NONNULL((1)) PCE_INLINE
void
pce_strbuf_putc(struct pce_strbuf *b, int c)
{
    if (b->p == b->e)
        pce_strbuf_reserve(b, 1);
    *b->p++ = c;
    *b->p = '\0';
}

/**
 * Write a string to the buffer.
 */
PCE_ATTR_NONNULL((1, 2)) PCE_INLINE
void
pce_strbuf_puts(struct pce_strbuf *b, const char *str)
{
    pce_strbuf_write(b, str, strlen(str));
}

/**
 * Write a formatted string to the buffer.
 */
PCE_ATTR_FORMAT(printf, 2, 3) PCE_ATTR_NONNULL((1, 2))
void
pce_strbuf_printf(struct pce_strbuf *b, const char *fmt, ...);

/**
 * Write a formatted string to the buffer.
 */
PCE_ATTR_NONNULL((1, 2))
void
pce_strbuf_vprintf(struct pce_strbuf *b, const char *fmt, va_list ap);

/** @} */

#ifdef __cplusplus
}
#endif
#endif
