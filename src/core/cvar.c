/* Copyright 2012-2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "cvar_private.h"
#include "private.h"
#include "sg/clock.h"
#include "sg/cvar.h"
#include "sg/entry.h"
#include "sg/error.h"
#include "sg/log.h"
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

const char SG_CVAR_DEFAULTSECTION[SG_CVAR_NAMELEN] = "general";

struct sg_cvartable sg_cvar_section;

enum {
    /* The cvar is being created.  */
    SG_CVAR_DEFINITION = 0200,

    /* The cvar has been set at some point.  */
    SG_CVAR_ISSET = 0400,

    /* Mask for flags passed to cvar definition functions.  */
    SG_CVAR_DEFMASK = (SG_CVAR_READONLY | SG_CVAR_INITONLY |
                       SG_CVAR_PERSISTENT),

    /* Mask for flags passed to cvar set functions.  */
    SG_CVAR_SETMASK = (SG_CVAR_PERSISTENT | SG_CVAR_CREATE |
                       SG_CVAR_IFUNSET)
};

/* Timer callback for saving the configuration.  */
static void
sg_cvar_timercallback(double time, void *cxt)
{
    struct sg_error *err = NULL;
    int r;

    (void) time;
    (void) cxt;

    r = sg_cvar_save("config.ini", strlen("config.ini"), 0, &err);
    if (r) {
        sg_logerrf(SG_LOG_ERROR, err, "Could not save configuration.");
        sg_error_clear(&err);
    } else {
        sg_logf(SG_LOG_INFO, "Configuration saved.");
    }
}

/* Mark the configuration as changed, so it will be saved to disk
   later.  The delay allows us to batch changes.  */
static void
sg_cvar_markdirty(void)
{
    sg_timer_register(
        1.0,
        SG_TIMER_RELTIME | SG_TIMER_KEEP_LAST,
        sg_cvar_timercallback,
        NULL);
}

/* Test whether a string is a member of a set.  The set is a string,
   with each element terminated by a NUL byte, and the set terminated
   by a second NUL byte.  */
static int
sg_cvar_strmemb(
    const char *set,
    const char *elem)
{
    const char *pset = set, *pelem;
    int cset, celem;
    while (*pset) {
        pelem = elem;
        while (1) {
            cset = *pset++;
            celem = *pelem++;
            if (celem >= 'A' && celem <= 'Z')
                celem += 'a' - 'A';
            if (celem != cset) {
                while (cset)
                    cset = *pset++;
                break;
            }
            if (!celem)
                return 1;
        }
    }
    return 0;
}

static union sg_cvar *
sg_cvar_get(
    const char *secbuf,
    const char *namebuf)
{
    struct sg_cvartable *sectable;
    sectable = sg_cvartable_get(&sg_cvar_section, secbuf);
    if (!sectable)
        return NULL;
    return sg_cvartable_get(sectable, namebuf);
}

static void **
sg_cvar_insert(
    const char *secbuf,
    const char *namebuf)
{
    void **secptr, **varptr;
    struct sg_cvartable *sectable;

    secptr = sg_cvartable_insert(&sg_cvar_section, secbuf);
    if (!secptr)
        goto panic;
    sectable = *secptr;
    if (!sectable) {
        sectable = malloc(sizeof(*sectable));
        if (!sectable)
            goto panic;
        sg_cvartable_init(sectable);
        *secptr = sectable;
    }

    varptr = sg_cvartable_insert(sectable, namebuf);
    if (!varptr)
        goto panic;

    return varptr;

panic:
    sg_sys_abort("out of memory");
    return NULL;
}

static int
sg_cvar_set_obj2(
    const char *section,
    const char *name,
    union sg_cvar *cvar,
    const char *value,
    unsigned flags)
{
    static const char
        BAD_INT[] = "invalid integer",
        BAD_FLOAT[] = "invalid floating-point number",
        BAD_BOOL[] = "invalid boolean";
    const char *msg;
    unsigned cflags = cvar->chead.flags, type = cflags >> 16;
    int clamped = 0, set_current, set_persistent;

    if (cflags & SG_CVAR_READONLY) {
        msg = "cvar is read-only";
        goto fail;
    }

    set_current = ((cflags & SG_CVAR_INITONLY) == 0 ||
                   (flags & SG_CVAR_DEFINITION) != 0);
    set_persistent = (flags & SG_CVAR_PERSISTENT) != 0;
    if (!set_current) {
        sg_logf(
            set_persistent ? SG_LOG_WARN : SG_LOG_ERROR,
            "Cvar %s.%s can only be set at startup.",
            section, name);
        if (!set_persistent)
            return -1;
    }
    if ((flags & SG_CVAR_IFUNSET) != 0 &&
        (cflags & SG_CVAR_ISSET) != 0) {
        set_current = 0;
        if (!set_persistent)
            return 0;
    }

    switch (type) {
    case SG_CVAR_STRING:
    case SG_CVAR_USER:
    {
        char *newstring, *prevstring;
        size_t len;
        len = strlen(value);
        if (len > SG_CVAR_VALUELEN) {
            msg = "value is too long";
            goto fail;
        }
        newstring = malloc(len + 1);
        if (!newstring) {
            sg_sys_abort("out of memory");
            return -1;
        }
        memcpy(newstring, value, len + 1);
        if (set_current) {
            prevstring = cvar->cstring.value;
            if (prevstring != cvar->cstring.persistent_value &&
                prevstring != cvar->cstring.default_value)
                free(prevstring);
            cvar->cstring.value = newstring;
        }
        if (set_persistent) {
            prevstring = cvar->cstring.persistent_value;
            if (prevstring != cvar->cstring.value &&
                prevstring != cvar->cstring.default_value)
                free(prevstring);
            cvar->cstring.persistent_value = newstring;
        }
    }
        break;

    case SG_CVAR_INT:
    {
        long newintl;
        int newint, ecode;
        char *end;

        if (!*value) {
            msg = BAD_INT;
            goto fail;
        }
        errno = 0;
        newintl = strtol(value, &end, 0);
        ecode = errno;
        if (*end) {
            msg = BAD_INT;
            goto fail;
        }
        if ((newintl == LONG_MAX && ecode == ERANGE) ||
            newintl > cvar->cint.max_value) {
            clamped = 1;
            newint = cvar->cint.max_value;
        } else if ((newintl == LONG_MIN && ecode == ERANGE) ||
                   newintl < cvar->cint.min_value) {
            clamped = 1;
            newint = cvar->cint.min_value;
        } else {
            newint = (int) newintl;
        }
        if (set_current)
            cvar->cint.value = newint;
        if (set_persistent)
            cvar->cint.persistent_value = newint;
    }
        break;

    case SG_CVAR_FLOAT:
    {
        double newfloat;
        int ecode;
        char *end;
        if (!*value) {
            msg = BAD_FLOAT;
            goto fail;
        }
        errno = 0;
        newfloat = strtod(value, &end);
        ecode = errno;
        if (*end || !isfinite(newfloat)) {
            msg = BAD_FLOAT;
            goto fail;
        }
        if ((newfloat == HUGE_VAL && ecode == ERANGE) ||
            newfloat > cvar->cfloat.max_value) {
            clamped = 1;
            newfloat = cvar->cfloat.max_value;
        } else if ((newfloat == -HUGE_VAL && ecode == ERANGE) ||
                   newfloat < cvar->cfloat.min_value) {
            clamped = 1;
            newfloat = cvar->cfloat.min_value;
        }
        if (set_current)
            cvar->cfloat.value = newfloat;
        if (set_persistent)
            cvar->cfloat.value = newfloat;
    }
        break;

    case SG_CVAR_BOOL:
    {
        int newbool;
        if (sg_cvar_strmemb("true\0yes\0on\0001\0", value)) {
            newbool = 1;
        } else if (sg_cvar_strmemb("false\0no\0off\0000\0", value)) {
            newbool = 0;
        } else {
            msg = BAD_BOOL;
            goto fail;
        }
        if (set_current)
            cvar->cbool.value = newbool;
        if (set_persistent)
            cvar->cbool.persistent_value = newbool;
    }
        break;

    default:
        sg_sys_abort("corrupted cvar");
        return -1;
    }

    if (clamped) {
        sg_logf(SG_LOG_WARN, "Value out of range for cvar %s.%s",
                section, name);
    }

    if (set_current) {
        cflags |= SG_CVAR_MODIFIED | SG_CVAR_ISSET;
    }
    if (set_persistent) {
        cflags |= SG_CVAR_HASPERSISTENT;
        sg_cvar_markdirty();
    }
    cvar->chead.flags = cflags;
    return 0;

fail:
    sg_logf(SG_LOG_WARN, "Could not set cvar %s.%s: %s",
            section, name, msg);
    return -1;
}

static int
sg_cvar_names(
    char *secbuf,
    const char *section,
    size_t sectionlen,
    char *namebuf,
    const char *name,
    size_t namelen)
{
    if (section == NULL || sectionlen == 0) {
        section = SG_CVAR_DEFAULTSECTION;
        sectionlen = strlen(section);
    }

    if (sg_cvartable_getkey(secbuf, section, sectionlen)) {
        sg_logf(SG_LOG_WARN, "Invalid cvar section name: \"%.*s\"",
                (int) sectionlen, section);
        return -1;
    }

    if (sg_cvartable_getkey(namebuf, name, namelen)) {
        sg_logf(SG_LOG_WARN, "Invalid cvar name: \"%.*s\"",
                (int) namelen, name);
        return -1;
    }

    return 0;
}

static void
sg_cvar_define(
    const char *section,
    const char *name,
    const char *doc,
    union sg_cvar *cvar)
{
    char secbuf[SG_CVAR_NAMELEN + 1], namebuf[SG_CVAR_NAMELEN + 1];
    void **varptr;
    unsigned type;
    union sg_cvar *prev;
    int r;

    r = sg_cvar_names(secbuf, section, section ? strlen(section) : 0,
                      namebuf, name, strlen(name));
    if (r)
        return;

    varptr = sg_cvar_insert(secbuf, namebuf);
    prev = *varptr;
    if (prev) {
        type = prev->chead.flags >> 16;
        if (type != SG_CVAR_USER) {
            sg_logf(SG_LOG_WARN, "Duplicate cvar definition: %s.%s",
                    secbuf, namebuf);
            return;
        }
        if ((prev->cstring.flags & SG_CVAR_HASPERSISTENT) != 0) {
            sg_cvar_set_obj2(
                secbuf, namebuf, cvar,
                prev->cstring.persistent_value,
                SG_CVAR_DEFINITION | SG_CVAR_PERSISTENT);
            if (strcmp(prev->cstring.value, prev->cstring.persistent_value)) {
                sg_cvar_set_obj2(
                    secbuf, namebuf, cvar,
                    prev->cstring.value,
                    SG_CVAR_DEFINITION);
            }
        } else {
            sg_cvar_set_obj2(
                secbuf, namebuf, cvar,
                prev->cstring.value,
                SG_CVAR_DEFINITION);
        }
        if (prev->cstring.value != prev->cstring.persistent_value &&
            prev->cstring.value != prev->cstring.default_value)
            free(prev->cstring.value);
        if (prev->cstring.persistent_value !=
            prev->cstring.default_value)
            free(prev->cstring.persistent_value);
        free(prev);
    }
    cvar->chead.doc = doc;
    *varptr = cvar;
    sg_cvar_markdirty();
}

void
sg_cvar_defstring(
    const char *section,
    const char *name,
    const char *doc,
    struct sg_cvar_string *cvar,
    const char *value,
    unsigned flags)
{
    char *cvalue;
    if (!value)
        value = "";
    cvalue = (char *) value;
    cvar->flags = (flags & SG_CVAR_DEFMASK) | (SG_CVAR_STRING << 16);
    cvar->value = cvalue;
    cvar->persistent_value = cvalue;
    cvar->default_value = value;
    sg_cvar_define(section, name, doc, (union sg_cvar *) cvar);
}

void
sg_cvar_defint(
    const char *section,
    const char *name,
    const char *doc,
    struct sg_cvar_int *cvar,
    int value,
    int min_value,
    int max_value,
    unsigned flags)
{
    cvar->flags = (flags & SG_CVAR_DEFMASK) | (SG_CVAR_INT << 16);
    cvar->value = value;
    cvar->persistent_value = value;
    cvar->default_value = value;
    cvar->min_value = min_value;
    cvar->max_value = max_value;
    sg_cvar_define(section, name, doc, (union sg_cvar *) cvar);
}

void
sg_cvar_deffloat(
    const char *section,
    const char *name,
    const char *doc,
    struct sg_cvar_float *cvar,
    double value,
    double min_value,
    double max_value,
    unsigned flags)
{
    cvar->flags = (flags & SG_CVAR_DEFMASK) | (SG_CVAR_FLOAT << 16);
    cvar->value = value;
    cvar->persistent_value = value;
    cvar->default_value = value;
    cvar->min_value = min_value;
    cvar->max_value = max_value;
    sg_cvar_define(section, name, doc, (union sg_cvar *) cvar);
}

void
sg_cvar_defbool(
    const char *section,
    const char *name,
    const char *doc,
    struct sg_cvar_bool *cvar,
    int value,
    unsigned flags)
{
    cvar->flags = (flags & SG_CVAR_DEFMASK) | (SG_CVAR_BOOL << 16);
    cvar->value = value;
    cvar->persistent_value = value;
    cvar->default_value = value;
    sg_cvar_define(section, name, doc, (union sg_cvar *) cvar);
}

int
sg_cvar_set(
    const char *fullname,
    size_t fullnamelen,
    const char *value,
    unsigned flags)
{
    const char *p;
    const char *section, *name;
    size_t sectionlen, namelen;
    char secbuf[SG_CVAR_NAMELEN + 1], namebuf[SG_CVAR_NAMELEN + 1];
    int r;

    p = memchr(fullname, '.', fullnamelen);
    if (p) {
        section = fullname;
        sectionlen = p - fullname;
        name = p + 1;
        namelen = fullname + fullnamelen - name;
    } else {
        section = fullname;
        sectionlen = 0;
        name = fullname;
        namelen = fullnamelen;
    }

    r = sg_cvar_names(secbuf, section, sectionlen, namebuf, name, namelen);
    if (r)
        return -1;

    return sg_cvar_set2(secbuf, namebuf, value, flags);
}

int
sg_cvar_set2(
    const char *section,
    const char *name,
    const char *value,
    unsigned flags)
{
    union sg_cvar *cvar;

    if (flags & SG_CVAR_CREATE) {
        void **varptr;
        struct sg_cvar_string *newvar;
        char *empty = (char *) "";
        varptr = sg_cvar_insert(section, name);
        cvar = *varptr;
        if (!cvar) {
            newvar = malloc(sizeof(*newvar));
            if (!newvar) {
                sg_sys_abort("out of memory");
                return -1;
            }
            newvar->flags = SG_CVAR_USER << 16;
            newvar->doc = NULL;
            newvar->value = empty;
            newvar->persistent_value = empty;
            newvar->default_value = empty;
            *varptr = newvar;
            cvar = (union sg_cvar *) newvar;
        }
    } else {
        cvar = sg_cvar_get(section, name);
        if (!cvar) {
            sg_logf(SG_LOG_WARN, "Cvar does not exist: %s.%s",
                    section, name);
            return -1;
        }
    }

    return sg_cvar_set_obj(section, name, cvar, value, flags);
}

int
sg_cvar_set_obj(
    const char *section,
    const char *name,
    union sg_cvar *cvar,
    const char *value,
    unsigned flags)
{
    return sg_cvar_set_obj2(section, name, cvar, value,
                            flags & SG_CVAR_SETMASK);
}

void
sg_cvar_loadcfg(void)
{
    struct sg_error *err = NULL;
    int r;

    r = sg_cvar_loadfile(
        "config", strlen("config"),
        SG_CVAR_CREATE | SG_CVAR_PERSISTENT | SG_CVAR_IFUNSET,
        &err);
    if (r) {
        if (err->domain != &SG_ERROR_NOTFOUND) {
            sg_logerrf(SG_LOG_ERROR, err,
                       "Could not load configuration file.");
        }
        sg_error_clear(&err);
    }

    sg_cvar_markdirty();
}
