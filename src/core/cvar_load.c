/* Copyright 2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "cvar_private.h"
#include "sg/cvar.h"
#include "sg/error.h"
#include "sg/file.h"
#include "sg/log.h"
#include <string.h>

#define SG_CVAR_MAXCFGSIZE (1024 * 1024)

int
sg_cvar_loadfile(
    const char *path,
    size_t pathlen,
    unsigned flags,
    struct sg_error **err)
{
    struct sg_filedata *data;
    int r;
    r = sg_file_load(&data, path, pathlen, 0, "ini", SG_CVAR_MAXCFGSIZE,
                     NULL, err);
    if (r)
        return -1;
    r = sg_cvar_loadbuffer(data, flags, err);
    sg_filedata_decref(data);
    return r;
}

static int
is_space(int c)
{
    return c == ' ' || c == '\t';
}

static int
is_alnum(int c)
{
    return (c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') ||
        c == '_';
}

static const char *
skip_space(const char *start)
{
    const char *ptr = start;
    while (is_space(*ptr))
        ptr++;
    return ptr;
}

static const char *
find_break(const char *start, const char *end)
{
    const char *ptr = start;
    while (ptr != end && *ptr != '\n' && *ptr != '\n')
        ptr++;
    return ptr;
}

int
sg_cvar_loadbuffer(
    struct sg_filedata *data,
    unsigned flags,
    struct sg_error **err)
{
    static const char
        ERR_SYNTAX[] = "Syntax error.",
        ERR_SECTION[] = "Invalid section name.",
        ERR_NAME[] = "Invalid cvar name.",
        ERR_VALUE[] = "Invalid cvar value.",
        ERR_ESCAPE[] = "Invalid escape sequence.",
        ERR_LONG[] = "Value is too long.",
        ERR_NOSECTION[] = "Missing configuration section.";
    int lineno = 0, error_count = 0, state = 0, r;
    const char *ptr = data->data, *end = ptr + data->length;
    char section[SG_CVAR_NAMELEN + 1];
    const char *msg;
    while (ptr != end) {
        const char *lstart, *lend;
        lineno++;
        ptr =
        lstart = skip_space(ptr);
        lend = find_break(ptr, end);
        ptr = lend;
        if (*ptr == '\r') {
            ptr++;
            if (*ptr == '\n')
                ptr++;
        } else if (*ptr == '\n') {
            ptr++;
        }
        if (*lstart == '#' || *lstart == ';' || lstart == lend)
            continue;

        if (is_alnum(*lstart)) {
            char name[SG_CVAR_NAMELEN + 1], value[SG_CVAR_VALUELEN + 1];
            const char *p;
            if (state == 0) {
                msg = ERR_NOSECTION;
                goto error;
            } else if (state < 0) {
                continue;
            }
            for (p = lstart; is_alnum(*p); p++) { }
            r = sg_cvartable_getkey(name, lstart, p - lstart);
            if (r) {
                msg = ERR_NAME;
                goto error;
            }
            for (; is_space(*p); p++) { }
            if (*p != '=') {
                msg = ERR_SYNTAX;
                goto error;
            }
            p++;
            for (; is_space(*p); p++) { }
            if (*p == '"') {
                char *vpos = value;
                p++;
                while (1) {
                    if (p == lend) {
                        msg = ERR_VALUE;
                        goto error;
                    }
                    int c = (unsigned char) *p++;
                    if (c == '"')
                        break;
                    if (vpos == value + SG_CVAR_VALUELEN) {
                        msg = ERR_LONG;
                        goto error;
                    }
                    if (c < 0x20) {
                        msg = ERR_VALUE;
                        goto error;
                    } else if (c == '\\') {
                        c = (unsigned char) *p++;
                        switch (c) {
                        case '\\': break;
                        case '"': break;
                        case 'n': c = '\n'; break;
                        case 't': c = '\t'; break;
                        default:
                            msg = ERR_ESCAPE;
                            goto error;
                        }
                    }
                    *vpos++ = c;
                }
                *vpos = '\0';
            } else {
                const char *vstart = p, *vend = p;
                for (; p != lend; p++) {
                    if (*p == ';' || *p == '#') {
                        break;
                    } else if (!is_space(*p)) {
                        vend = p + 1;
                    }
                }
                if (vend - vstart > SG_CVAR_VALUELEN) {
                    msg = ERR_LONG;
                    goto error;
                }
                memcpy(value, vstart, vend - vstart);
                value[vend - vstart] = '\0';
            }
            sg_cvar_set2(section, name, value, flags);
        } else if (*lstart == '[') {
            const char *sstart, *send, *p;
            state = -1;
            sstart = skip_space(lstart + 1);
            for (p = sstart; p != lend && *p != ']'; p++) { }
            if (p == lend) {
                msg = ERR_SYNTAX;
                goto error;
            }
            for (send = p; send != sstart && is_space(send[-1]); send--) { }
            r = sg_cvartable_getkey(section, sstart, send - sstart);
            if (r) {
                msg = ERR_SECTION;
                goto error;
            }
            state = 1;
            p++;
            for (; p != lend; p++) {
                if (*p == '#' || *p == ';')
                    break;
                if (!is_space(*p)) {
                    msg = ERR_SYNTAX;
                    goto error;
                }
            }
        } else {
            msg = ERR_SYNTAX;
            goto error;
        }
        continue;

    error:
        sg_logf(SG_LOG_ERROR, "%s:%d: %s", data->path, lineno, msg);
        error_count += 1;
        if (error_count >= 10) {
            sg_error_data(err, "ini");
            return -1;
        }
    }

    return 0;
}
