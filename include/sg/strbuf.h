#ifndef BASE_STRBUF_H
#define BASE_STRBUF_H
#include "defs.h"
#include <assert.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#ifdef __cplusplus
extern "C" {
#endif
/* A strbuf is a buffer with a known size.  There is always one extra
   byte in the underlying memory containing NUL, this is not counted
   in the length.  The extra byte is just for convenience.  Functions
   which write to string buffers automatically allocate enough memory
   to store the result.

   This is inspired by the strbuf API in Git. */

struct sg_strbuf {
    char *s, *p, *e;
};

/* Initialize an empty string buffer.  */
void
sg_strbuf_init(struct sg_strbuf *b, size_t initsz);

/* Destroy a string buffer and free associated memory.  The strbuf is
   reset to a zero-length string.  */
void
sg_strbuf_destroy(struct sg_strbuf *b);

/* Initialize a buffer with the given string which is in a region of
   memory allocated with malloc which has the given length.  The
   length must be at least one byte longer than the string, for the
   NUL byte.  */
void
sg_strbuf_attach(struct sg_strbuf *b, char *str, size_t len);

/* Return the string in the buffer, which must be later freed with
   free().  The strbuf is reset to a zero-length string.  */
char *
sg_strbuf_detach(struct sg_strbuf *b);

/* Get the amount of unused space remaining in the buffer.  */
SG_INLINE size_t
sg_strbuf_avail(struct sg_strbuf *b)
{
    return b->e - b->p;
}

/* Allocate available space in the buffer.  */
void
sg_strbuf_reserve(struct sg_strbuf *b, size_t len);

/* Shrink the buffer to match the current contents.  */
void
sg_strbuf_compact(struct sg_strbuf *b);

/* Set the string to the empty string, but don't free memory.  */
SG_INLINE void
sg_strbuf_clear(struct sg_strbuf *b)
{
    b->p = b->s;
    *b->p = '\0';
}

/* Equivalent to calling strcmp on the contained strings.  */
int
sg_strbuf_cmp(struct sg_strbuf *x, struct sg_strbuf *y);

/* Get the length of the string in the buffer.  */
SG_INLINE size_t
sg_strbuf_len(struct sg_strbuf *b)
{
    return b->p - b->s;
}

/* Set the length of the string in the buffer.  This must not be
   longer than the current length.  */
SG_INLINE void
sg_strbuf_setlen(struct sg_strbuf *b, size_t len)
{
    size_t l = b->p - b->s;
    assert(len <= l);
    b->p = b->s + len;
    *b->p = '\0';
}

/* Set the length of the string in the buffer.  This will expand the
   string as necessary, and assumes that you have already reserved the
   space and written the data.  */
SG_INLINE void
sg_strbuf_forcelen(struct sg_strbuf *b, size_t len)
{
    size_t l = b->e - b->s;
    assert(len <= l);
    b->p = b->s + len;
    *b->p = '\0';
}

/* Strbuf I/O functions */

/* Add a buffer to the strbuf.  */
void
sg_strbuf_write(struct sg_strbuf *b, const char *data, size_t len);

/* Add a character to the buffer.  */
SG_INLINE void
sg_strbuf_putc(struct sg_strbuf *b, int c)
{
    if (b->p == b->e)
        sg_strbuf_reserve(b, 1);
    *b->p++ = c;
    *b->p = '\0';
}

/* Add a string to the buffer.  */
SG_INLINE void
sg_strbuf_puts(struct sg_strbuf *b, const char *str)
{
    sg_strbuf_write(b, str, strlen(str));
}

/* Format a string into the buffer.  */
__attribute__((format(printf, 2, 3)))
void
sg_strbuf_printf(struct sg_strbuf *b, const char *fmt, ...);

/* Format a string into the buffer.  */
void
sg_strbuf_vprintf(struct sg_strbuf *b, const char *fmt, va_list ap);

/* Path functions: These append relative paths to existing paths.  The
   resulting changes will be normalized, but the existing path will
   not be normalized.  It is assumed that the existing path is already
   normalized, but nothing will break if it is not.  For example,

   "aa" + "../bb" -> "bb"
   "aa" + "bb//cc" -> "aa/bb/cc"
   "aa" + "bb/" -> "aa/bb"
   "aa" + "." -> "aa"
   "aa" + "/bb" -> "/bb"
   "aa" + "../.." -> ".." */

void
sg_strbuf_joinmem(struct sg_strbuf *buf, const char *path, size_t len);

SG_INLINE void
sg_strbuf_joinstr(struct sg_strbuf *buf, const char *path)
{
    sg_strbuf_joinmem(buf, path, strlen(path));
}

/* Change a path into the path containing it.  This is the same as
   joining ".." to the buffer.  */
void
sg_strbuf_getdir(struct sg_strbuf *buf);

#ifdef __cplusplus
}
#endif
#endif
