/* Copyright 2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "cvar_private.h"
#include "sg/cvar.h"
#include "sg/entry.h"
#include "sg/error.h"
#include "sg/file.h"
#include "sg/log.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static int
sg_cvar_cmpname(const void *x, const void *y)
{
    return memcmp(x, y, SG_CVAR_NAMELEN);
}

static int
sg_cvar_save_str(struct sg_textwriter *wp, const char *secname,
                 const char *name, const char *value)
{
    size_t len = strlen(value), i;
    char buf[SG_CVAR_VALUELEN * 2 + 3], *pos, *qval;
    int needs_quote, c;

    if (!len)
        return sg_textwriter_putf(wp, "%s =", name);
    if (len > SG_CVAR_VALUELEN) {
        sg_logf(SG_LOG_WARN, "Cvar value too long: %s.%s", secname, name);
        goto failed;
    }

    needs_quote = 0;
    pos = buf;
    *pos++ = '"';
    for (i = 0; i < len; i++) {
        c = (unsigned char) value[i];
        if (c < 0x20) {
            needs_quote = 1;
            *pos++ = '\\';
            switch (c) {
            case '\t': *pos++ = '\t'; break;
            case '\n': *pos++ = '\n'; break;
            default:
                goto failed_badchar;
            }
        } else if (c < 0x80) {
            if (c == '\\' || c == '"') {
                needs_quote = 1;
                *pos++ = '\\';
            } else if (c == '#' || c == ';') {
                needs_quote = 1;
            }
            *pos++ = c;
        } else {
            /* FIXME: only emit valid UTF-8 and no control characters */
            *pos++ = c;
        }
    }
    if (pos[-1] == ' ' || buf[1] == ' ') {
        needs_quote = 1;
    }
    if (needs_quote) {
        *pos++ = '"';
        qval = buf;
    } else {
        qval = buf + 1;
    }
    *pos++ = '\0';
    return sg_textwriter_putf(wp, "%s = %s", name, qval);

failed_badchar:
    sg_logf(SG_LOG_WARN, "Cvar contains invalid characters: %s.%s",
            secname, name);
    goto failed;

failed:
    return sg_textwriter_putf(wp, "# %s", name);
}

int
sg_cvar_save(
    const char *path,
    size_t pathlen,
    int save_all,
    struct sg_error **err)
{
    const char *secname, *cvarname;
    char (*names)[SG_CVAR_NAMELEN] = NULL, (*key)[SG_CVAR_NAMELEN];
    struct sg_cvartable *section;
    unsigned count = sg_cvar_section.count, size = sg_cvar_section.size,
        scount, ssize, i, j, k, max_scount;
    struct sg_textwriter wp;
    void **sp, **se;
    int r, state;
    struct sg_cvar_head *cvar;

    r = sg_textwriter_open(&wp, path, pathlen, err);
    if (r)
        return -1;

    if (!count)
        goto done;
    max_scount = 0;
    for (sp = sg_cvar_section.value, se = sp + size; sp != se; sp++) {
        section = *sp;
        if (!section)
            continue;
        if (section->count > max_scount)
            max_scount = section->count;
    }

    names = malloc(SG_CVAR_NAMELEN * (count + max_scount));
    if (!names)
        goto nomem;
    key = sg_cvar_section.key;
    for (i = 0, j = 0; i < size; i++) {
        if (!key[i][0])
            continue;
        if (!strcmp(key[i], SG_CVAR_DEFAULTSECTION))
            memset(names[j], '\0', SG_CVAR_NAMELEN);
        else
            memcpy(names[j], key[i], SG_CVAR_NAMELEN);
        j++;
    }
    assert(j == count);
    qsort(names, count, SG_CVAR_NAMELEN, sg_cvar_cmpname);

    state = 0;
    for (k = 0; k < count; k++) {
        state = state != 0;
        secname = names[k][0] ? names[k] : SG_CVAR_DEFAULTSECTION;
        section = sg_cvartable_get(&sg_cvar_section, secname);
        assert(section != NULL);
        key = section->key;
        scount = section->count;
        ssize = section->size;
        assert(scount <= max_scount);
        for (i = 0, j = 0; i < ssize; i++) {
            if (!key[i][0])
                continue;
            memcpy(names[count + j], key[i], SG_CVAR_NAMELEN);
            j++;
        }
        assert(j == scount);
        qsort(names + count, scount, SG_CVAR_NAMELEN, sg_cvar_cmpname);

        for (j = 0; j < scount; j++) {
            cvarname = names[count + j];
            cvar = sg_cvartable_get(section, cvarname);
            assert(cvar != NULL);
            if ((cvar->flags & SG_CVAR_PERSISTENT) == 0 && !save_all)
                continue;
            if (state < 2) {
                if (state == 1)
                    sg_textwriter_putc(&wp, '\n');
                sg_textwriter_putf(&wp, "[%s]\n", secname);
                state = 2;
            } else {
                sg_textwriter_putc(&wp, '\n');
            }
            if (cvar->doc)
                sg_textwriter_putf(&wp, "# %s\n", cvar->doc);
            if ((cvar->flags & SG_CVAR_HASPERSISTENT) == 0)
                sg_textwriter_puts(&wp, "# ");
            switch (cvar->flags >> 16) {
            case SG_CVAR_STRING:
            case SG_CVAR_USER:
                sg_cvar_save_str(
                    &wp, secname, cvarname,
                    ((struct sg_cvar_string *) cvar)->persistent_value);
                break;

            case SG_CVAR_INT:
                sg_textwriter_putf(
                    &wp, "%s = %d", cvarname,
                    ((struct sg_cvar_int *) cvar)->persistent_value);
                break;

            case SG_CVAR_FLOAT:
                sg_textwriter_putf(
                    &wp, "%s = %f", cvarname,
                    ((struct sg_cvar_float *) cvar)->persistent_value);
                break;

            case SG_CVAR_BOOL:
                sg_textwriter_putf(
                    &wp, "%s = %s", cvarname,
                    ((struct sg_cvar_bool *) cvar)->persistent_value ?
                    "yes" : "no");
                break;

            default:
                sg_sys_abort("corrupted cvar");
                goto done;
            }
            sg_textwriter_putc(&wp, '\n');
        }
    }

done:
    free(names);
    r = sg_textwriter_commit(&wp, err);
    sg_textwriter_close(&wp);
    return r;

nomem:
    free(names);
    sg_textwriter_close(&wp);
    sg_error_nomem(err);
    return -1;
}
