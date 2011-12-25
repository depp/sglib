#include "strbuf.h"
#include <stdio.h>
#include <stdlib.h>

/* FIXME: abort() should be something else.  */

static char sg_strbuf_static[1];

#ifndef SG_STRBUF_ZERO
#define SG_STRBUF_ZERO 1
#endif

static void
sg_strbuf_zero(struct sg_strbuf *b)
{
    b->s = b->p = b->e = NULL;
}

void
sg_strbuf_init(struct sg_strbuf *b, size_t initsz)
{
    b->s = b->p = b->e = sg_strbuf_static;
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
    sg_strbuf_zero(b);
    return p;
}

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

__attribute__((format(printf, 2, 3)))
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
    len = len < 0 : 0 : len;
    if (len < rem)
        sg_strbuf_reserve(b, len);
    _vsnprintf(b->p, rem, fmt, ap);
    b->p += len;
    *b->p = '\0';
}

#endif

void
sg_strbuf_joinmem(struct sg_strbuf *buf, const char *path, size_t len)
{
    const char *p = path, *e = path + len, *q;
    size_t l = sg_strbuf_len(buf), cl;
    if (p != e && *p == '/') {
        do p++; while (*p == '/' && p != e);
        sg_strbuf_clear(buf);
        sg_strbuf_putc(buf, '/');
    } else if (l == 0) {
        sg_strbuf_putc(buf, '.');
    }
    goto first;

next:
    for (p = q; p != e && *p == '/'; ++p) { }
first:
    if (p == e)
        return;
    for (q = p; q != e && *q != '/'; ++q) { }
    cl = q - p;
    if (cl <= 2) {
        if (cl == 1 && p[0] == '.') {
            goto next;
        } else if (cl == 2 && p[0] == '.' && p[1] == '.') {
            sg_strbuf_getdir(buf);
            goto next;
        }
    }
    if (*buf->s == '.' && buf->p == buf->s + 1)
        sg_strbuf_clear(buf);
    else if (buf->p[-1] != '/')
        sg_strbuf_putc(buf, '/');
    sg_strbuf_write(buf, p, q - p);
    goto next;
}

void
sg_strbuf_getdir(struct sg_strbuf *buf)
{
    char *pp;
    size_t l = buf->p - buf->s;
    if (l <= 1) {
        if (l == 0 || *buf->s == '.') {
            sg_strbuf_putc(buf, '.');
            return;
        }
    }
    pp = buf->p - 1;
    while (pp != buf->s && *pp != '/')
        --pp;
    buf->p = pp;
    if (pp == buf->s)
        sg_strbuf_putc(buf, *pp == '/' ? '/' : '.');
    else
        *pp = '\0';
}
