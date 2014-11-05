/* Copyright 2011-2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "sg/strbuf.h"
#include <stdio.h>
#include <stdlib.h>

/* FIXME: abort() should be something else.  */

/*
  This can't be const because it's written to by strbuf_clear(), to
  make strbuf_clear() simpler.
*/
static char sg_strbuf_static[1] = { 0 };

static void
sg_strbuf_zero(struct sg_strbuf *b)
{
    b->s = b->p = b->e = sg_strbuf_static;
}

void
sg_strbuf_init(struct sg_strbuf *b, size_t initsz)
{
    sg_strbuf_zero(b);
    if (initsz)
        sg_strbuf_reserve(b, initsz);
}

void
sg_strbuf_destroy(struct sg_strbuf *b)
{
    if (b->s != sg_strbuf_static)
        free(b->s);
    sg_strbuf_zero(b);
}

void
sg_strbuf_attach(struct sg_strbuf *b, char *str, size_t len)
{
    b->s = str;
    b->p = b->e = str + len;
}

char *
sg_strbuf_detach(struct sg_strbuf *b)
{
    char *p = b->s;
    if (p == sg_strbuf_static) {
        p = malloc(1);
        if (!p)
            abort();
        *p = '\0';
    }
    sg_strbuf_zero(b);
    return p;
}

/* If doubling the buffer gives us enough space, then double the
   buffer.  Otherwise, allocate exactly as much space as
   requested.  */
void
sg_strbuf_reserve(struct sg_strbuf *b, size_t len)
{
    size_t rem = b->e - b->p, minsz, buflen, bufsz, nsz;
    char *np;
    if (len <= rem)
        return;
    buflen = b->p - b->s;
    bufsz = (b->e - b->s) + 1;
    minsz = buflen + len + 1;
    nsz = bufsz * 2;
    if (nsz < minsz)
        nsz = minsz;
    if (nsz < 16)
        nsz = 16;
    np = malloc(nsz);
    if (!np)
        abort();
    memcpy(np, b->s, buflen + 1);
    if (b->s != sg_strbuf_static)
        free(b->s);
    b->s = np;
    b->p = np + buflen;
    b->e = np + nsz - 1;
}

void
sg_strbuf_compact(struct sg_strbuf *b)
{
    char *p;
    size_t sz;
    /* This is also true for static allocations.  */
    if (b->p == b->e)
        return;
    sz = b->p - b->s;
    p = realloc(b->s, sz + 1);
    (void) p;
    b->e = b->p;
}

int
sg_strbuf_cmp(struct sg_strbuf *x, struct sg_strbuf *y)
{
    int c;
    size_t xl = sg_strbuf_len(x), yl = sg_strbuf_len(y);
    if (xl < yl) {
        c = memcmp(x->s, y->s, xl);
        return c ? c : -1;
    } else if (xl > yl) {
        c = memcmp(x->s, y->s, yl);
        return c ? c : 1;
    } else {
        return memcmp(x->s, y->s, xl);
    }
}

void
sg_strbuf_write(struct sg_strbuf *b, const char *data, size_t len)
{
    size_t rem = b->e - b->p;
    if (len > rem)
        sg_strbuf_reserve(b, len);
    memcpy(b->p, data, len);
    b->p += len;
    *b->p = '\0';
}

void
sg_strbuf_printf(struct sg_strbuf *b, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    sg_strbuf_vprintf(b, fmt, ap);
    va_end(ap);
}

#if !defined(_WIN32)

void
sg_strbuf_vprintf(struct sg_strbuf *b, const char *fmt, va_list ap)
{
    int r;
    size_t rem = b->e - b->p;
    va_list ap2;
    if (!rem) {
        sg_strbuf_reserve(b, 64);
        rem = b->e - b->p;
    }
    va_copy(ap2, ap);
    r = vsnprintf(b->p, rem + 1, fmt, ap2);
    va_end(ap2);
    if (r < 0)
        abort();
    if ((size_t) r > rem) {
        sg_strbuf_reserve(b, r);
        rem = b->e - b->p;
        r = vsnprintf(b->p, rem + 1, fmt, ap);
        if (r < 0)
            abort();
        if ((size_t) r > rem)
            abort();
    }
    b->p += r;
    /* vsnprintf wrote the '\0' */
}

#else

void
sg_strbuf_vprintf(struct sg_strbuf *b, const char *fmt, va_list ap)
{
    size_t rem;
    int len;
    rem = b->e - b->p;
    len = _vscprintf(fmt, ap);
    len = len < 0 ? 0 : len;
    if ((size_t) len > rem)
        sg_strbuf_reserve(b, len);
    _vsnprintf_s(b->p, rem, _TRUNCATE, fmt, ap);
    b->p += len;
    *b->p = '\0';
}

#endif
