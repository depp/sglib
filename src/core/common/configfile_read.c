/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "sg/configfile.h"
#include "sg/file.h"
#include "sg/log.h"
#include <stddef.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

static int is_white(int c)
{
    return c == ' ' || c == '\t';
}

static int is_ident(int c)
{
    return (c >= 'A' && c <= 'Z') ||
        (c >= 'a' && c <= 'z') ||
        (c >= '0' && c <= '9') ||
        c == '_' || c == '-';
}

static int is_value(int c)
{
    return c >= 32 && c <= 0x126;
}

#define MAX_CONFIGFILE (100 * 1024)
#define MAX_WARNING 5
#define restrict

int
configfile_read(struct configfile *f, const char *path)
{
    static struct sg_logger *logger;
    const unsigned char *restrict ptr, *end;
    const unsigned char *sptr = NULL, *nptr = NULL, *vptr = NULL;
    unsigned slen = 0, nlen = 0, vlen = 0, len;
    struct sg_buffer *fbuf;
    int r = 0, lineno, warning = 0, quoted, c;
    const char *msg;
    char *buf = NULL, *bptr, *sstr, *nstr, *vstr;
    size_t bufsize = 0;

    fbuf = sg_file_get(path, strlen(path), 0, NULL,
                       MAX_CONFIGFILE, NULL);
    if (!fbuf)
        return -1;

    ptr = fbuf->data;
    end = ptr + fbuf->length;
    lineno = 1;

initial:
    while (1) {
        while (is_white(*ptr))
            ptr++;
        if (ptr == end)
            goto done;
        if (*ptr == '\r') {
            if (*(ptr + 1) == '\n')
                ptr += 2;
            lineno += 1;
        } else if (*ptr == '\n') {
            lineno += 1;
        } else {
            break;
        }
    }
    if (*ptr == '[')
        goto section;
    if (is_ident(*ptr))
        goto variable;
    if (*ptr == ';' || *ptr == '#')
        goto skipline;
    goto badchar;

section:
    ptr++;
    while (is_white(*ptr))
        ptr++;
    if (!is_ident(*ptr))
        goto badchar;
    sptr = ptr;
    ptr++;
    while (is_ident(*ptr) || *ptr == '.')
        ptr++;
    slen = (unsigned) (ptr - sptr);
    while (is_white(*ptr))
        ptr++;
    if (*ptr != ']') {
        msg = "missing ]";
        goto error;
    }
    ptr++;
    while (is_white(*ptr))
        ptr++;
    if (*ptr == ';' || *ptr == '#')
        goto skipline;
    if (ptr == end)
        goto done;
    if (*ptr == '\r' || *ptr == '\n')
        goto initial;
    goto badchar;

variable:
    nptr = ptr;
    ptr++;
    while (is_ident(*ptr) || *ptr == '.')
        ptr++;
    nlen = (unsigned) (ptr - nptr);
    while (is_white(*ptr))
        ptr++;
    if (*ptr != '=') {
        msg = "missing =";
        goto error;
    }
    ptr++;
    while (is_white(*ptr))
        ptr++;
    quoted = 0;
    if (is_value(*ptr)) {
        if (*ptr == '"') {
            quoted = 1;
            ptr++;
            vptr = ptr;
            while (1) {
                if (!is_value(*ptr))
                    goto badchar;
                if (*ptr == '\\') {
                    ptr++;
                    if (!is_value(*ptr))
                        goto badchar;
                } else if (*ptr == '"') {
                    break;
                }
                ptr++;
            }
            vlen = (unsigned) (ptr - vptr);
            ptr++;
        } else {
            vptr = ptr;
            while (is_value(*ptr) && *ptr != ';' && *ptr != '#')
                ptr++;
            vlen = (unsigned) (ptr - vptr);
            while (vlen && is_white(ptr[vlen - 1]))
                vlen--;
        }
    } else if (*ptr == '\n' || *ptr == '\r') {
        vptr = NULL;
        vlen = 0;
    }

    /* Fill buffer with section, name, and value
       We concatenate: section '.' name '\0' value '\0'
       Then we split section.name at the last '.'.  */
    len = slen + nlen + vlen + 4;
    if (bufsize < len) {
        if (!bufsize)
            bufsize = 64;
        while (bufsize < len)
            bufsize *= 2;
        free(buf);
        buf = malloc(bufsize);
        if (!buf) {
            r = ENOMEM;
            goto done;
        }
    }

    bptr = buf;
    /* Copy section and name */
    sstr = bptr;
    if (slen) {
        memcpy(bptr, sptr, slen);
        bptr += slen;
        *bptr = '.';
        bptr++;
    }
    memcpy(bptr, nptr, nlen);
    bptr += nlen;
    *bptr = '\0';
    nstr = bptr;
    while (nstr != buf && *(nstr - 1) != '.')
        nptr--;
    if (nstr == buf) {
        msg = "variable has no section";
        goto error;
    }
    if (nstr == bptr) {
        msg = "variable has no name";
        goto error;
    }
    *(nstr - 1) = '\0';
    bptr++;

    /* Copy value */
    vstr = bptr;
    if (quoted) {
        while (1) {
            c = *vptr;
            if (c == '"')
                break;
            if (c == '\\') {
                vptr++;
                switch ((c = *vptr)) {
                case '\\':
                case '"':
                    break;

                case 'n':
                    c = '\n';
                    break;

                case 'r':
                    c = '\r';
                    break;

                case 't':
                    c = '\t';
                    break;

                default:
                    msg = "invalid escape sequence";
                    goto error;
                }
            }
            *bptr = c;
            bptr++;
            vptr++;
        }
    } else {
        memcpy(bptr, vptr, vlen);
        bptr += vlen;
    }
    *bptr = '\0';

    r = configfile_set(f, sstr, nstr, vstr);
    if (r)
        goto done;

skipline:
    while (*ptr != '\n' && *ptr != '\r' && ptr != end)
        ptr++;
    goto initial;

badchar:
    if (ptr == end)
        msg = "unexpected end of file";
    else
        msg = "unexpected character";
    goto error;

error:
    if (!logger)
        logger = sg_logger_get("config");
    sg_logf(logger, LOG_ERROR, "%s:%d: %s", path, lineno, msg);
    if (++warning == MAX_WARNING) {
        sg_logf(logger, LOG_ERROR, "%s: too many errors, aborting");
        r = -1;
        goto done;
    }
    goto skipline;

done:
    sg_buffer_decref(fbuf);
    free(buf);
    return r;
}
