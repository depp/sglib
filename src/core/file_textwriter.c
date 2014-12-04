/* Copyright 2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "sg/error.h"
#include "sg/file.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SG_TEXTWRITER_BUFSZ 8192

static int
sg_textwriter_writeall(
    struct sg_textwriter *wp,
    const char *buf,
    const char *end)
{
    struct sg_writer *fp = wp->fp;
    const char *ptr = buf;
    int r;
    while (ptr != end) {
        r = sg_writer_write(fp, ptr, end - ptr, &wp->err);
        if (r < 0)
            return -1;
        ptr += r;
    }
    return 0;
}

int
sg_textwriter_open(
    struct sg_textwriter *wp,
    const char *path,
    size_t pathlen,
    struct sg_error **err)
{
    struct sg_writer *fp;
    char *buf;
    buf = malloc(SG_TEXTWRITER_BUFSZ);
    if (!buf) {
        sg_error_nomem(err);
        return -1;
    }
    fp = sg_writer_open(path, pathlen, err);
    if (!wp->fp) {
        free(wp->buf);
        return -1;
    }
    wp->fp = fp;
    wp->buf = buf;
    wp->ptr = buf;
    wp->err = NULL;
    return 0;
}

int
sg_textwriter_commit(
    struct sg_textwriter *wp,
    struct sg_error **err)
{
    int r;
    if (!wp->fp) {
        sg_error_invalid(err, __FUNCTION__, "wp");
        return -1;
    }
    r = wp->err != NULL;
    if (!r) {
        r = sg_textwriter_writeall(wp, wp->buf, wp->ptr);
    }
    if (!r) {
        r = sg_writer_commit(wp->fp, err);
    } else {
        sg_error_move(err, &wp->err);
    }
    sg_textwriter_close(wp);
    return r;
}

void
sg_textwriter_close(
    struct sg_textwriter *wp)
{
    if (wp->fp == NULL)
        return;
    sg_writer_close(wp->fp);
    free(wp->buf);
    wp->fp = NULL;
    wp->buf = NULL;
    wp->ptr = NULL;
}

static int
sg_textwriter_write(
    struct sg_textwriter *wp,
    const char *buf,
    size_t len)
{
    size_t rem;
    int r;
    if (!wp->fp) {
        sg_error_invalid(&wp->err, __FUNCTION__, "wp");
        return -1;
    }
    if (wp->err) {
        return -1;
    }
    rem = (wp->buf + SG_TEXTWRITER_BUFSZ) - wp->ptr;
    if (len <= rem) {
        memcpy(wp->ptr, buf, len);
        wp->ptr += len;
        return 0;
    } else if (len < SG_TEXTWRITER_BUFSZ) {
        memcpy(wp->ptr, buf, rem);
        r = sg_textwriter_writeall(
            wp, wp->buf, wp->buf + SG_TEXTWRITER_BUFSZ);
        if (r)
            return r;
        memcpy(wp->buf, buf + rem, len - rem);
        wp->ptr = wp->buf + (len - rem);
        return 0;
    } else {
        r = sg_textwriter_writeall(wp, wp->buf, wp->ptr);
        if (r)
            return r;
        r = sg_textwriter_writeall(wp, buf, buf + len);
        if (r)
            return r;
        wp->ptr = wp->buf;
        return 0;
    }
}

int
sg_textwriter_putc(
    struct sg_textwriter *wp,
    int c)
{
    char buf[4];
    size_t len;
    if (c <= 0) {
        goto badc;
    } else if (c < 0x80) {
        buf[0] = c;
        len = 1;
    } else if (c < 0x800) {
        buf[0] = 0xc0 | (c >> 6);
        buf[1] = 0x80 | (c & 0x3f);
        len = 2;
    } else if (c < 0x10000) {
        if ((c & 0xf800) == 0xd800)
            goto badc;
        buf[0] = 0xe0 | (c >> 12);
        buf[1] = 0x80 | ((c >> 6) & 0x3f);
        buf[2] = 0x80 | (c & 0x3f);
        len = 3;
    } else if (c < 0x10ffff) {
        buf[0] = 0xf0 | (c >> 18);
        buf[1] = 0x80 | ((c >> 12) & 0x3f);
        buf[2] = 0x80 | ((c >> 6) & 0x3f);
        buf[3] = 0x80 | (c & 0x3f);
        len = 4;
    } else {
        goto badc;
    }
    return sg_textwriter_write(wp, buf, len);

badc:
    sg_error_invalid(&wp->err, __FUNCTION__, "c");
    return -1;
}

int
sg_textwriter_puts(
    struct sg_textwriter *wp,
    const char *str)
{
    return sg_textwriter_write(wp, str, strlen(str));
}

int
sg_textwriter_putmem(
    struct sg_textwriter *wp,
    const char *ptr,
    size_t len)
{
    return sg_textwriter_write(wp, ptr, len);
}

int
sg_textwriter_putf(
    struct sg_textwriter *wp,
    const char *fmt,
    ...)
{
    va_list ap;
    int r;
    va_start(ap, fmt);
    r = sg_textwriter_putv(wp, fmt, ap);
    va_end(ap);
    return r;
}

#if defined _WIN32

int
sg_textwriter_putv(
    struct sg_textwriter *wp,
    const char *fmt,
    va_list ap)
{
    char *ptr;
    size_t rem;
    int len, r;

    if (!wp->fp) {
        sg_error_invalid(&wp->err, __FUNCTION__, "wp");
        return -1;
    }
    if (wp->err) {
        return -1;
    }

    rem = (wp->buf + SG_TEXTWRITER_BUFSZ) - wp->ptr;
    len = _vscprintf(fmt, ap);
    len = len < 0 ? 0 : len;
    if ((size_t) len > rem) {
        r = sg_textwriter_writeall(wp, wp->buf, wp->ptr);
        if (r)
            return r;
        wp->ptr = wp->buf;
        if ((size_t) len > SG_TEXTWRITER_BUFSZ) {
            ptr = malloc(len);
            if (!ptr) {
                sg_error_nomem(&wp->err);
                return -1;
            }
            _vsnprintf_s(ptr, len, _TRUNCATE, fmt, ap);
            r = sg_textwriter_writeall(wp, ptr, ptr + len);
            free(ptr);
            return r;
        }
    }
    _vsnprintf_s(wp->ptr, (wp->buf + SG_TEXTWRITER_BUFSZ) - wp->ptr,
                 _TRUNCATE, fmt, ap);
    wp->ptr += len;
    return 0;
}

#else

int
sg_textwriter_putv(
    struct sg_textwriter *wp,
    const char *fmt,
    va_list ap)
{
    char *ptr;
    size_t rem;
    int len, r;

    if (!wp->fp) {
        sg_error_invalid(&wp->err, __FUNCTION__, "wp");
        return -1;
    }
    if (wp->err) {
        return -1;
    }

    rem = (wp->buf + SG_TEXTWRITER_BUFSZ) - wp->ptr;
    if (rem < 64) {
        r = sg_textwriter_writeall(wp, wp->buf, wp->ptr);
        if (r)
            return r;
        wp->ptr = wp->buf;
        rem = SG_TEXTWRITER_BUFSZ;
    }
    len = vsnprintf(wp->ptr, rem, fmt, ap);
    if (len < 0)
        abort();
    if ((size_t) len <= rem) {
        wp->ptr += len;
        return 0;
    }
    r = sg_textwriter_writeall(wp, wp->buf, wp->ptr);
    if (r)
        return r;
    if ((size_t) len > SG_TEXTWRITER_BUFSZ) {
        wp->ptr = wp->buf;
        ptr = malloc(len + 1);
        if (!ptr) {
            sg_error_nomem(&wp->err);
            return -1;
        }
        vsnprintf(ptr, len + 1, fmt, ap);
        r = sg_textwriter_writeall(wp, ptr, ptr + len);
        free(ptr);
        return r;
    }
    vsnprintf(wp->buf, SG_TEXTWRITER_BUFSZ, fmt, ap);
    wp->ptr = wp->buf + len;
    return 0;
}

#endif
