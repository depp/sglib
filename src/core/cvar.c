/* Copyright 2012-2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "sg/cvar.h"
#include "sg/configfile.h"

#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct sg_configfile sg_cvar_conf, sg_cvar_arg;

static int
to_lower(int c)
{
    if (c >= 'A' && c <= 'Z')
        return c + ('a' - 'A');
    return c;
}

void
sg_cvar_addarg(const char *section, const char *name, const char *value)
{
    char buf[128];
    const char *p, *sptr, *nptr, *vptr;
    size_t len;
    int r;

    if (!name) {
        p = strchr(value, '=');
        if (!p)
            goto invalid;
        len = p - value;
        if (len >= sizeof(buf))
            goto invalid;
        memcpy(buf, value, len);
        buf[len] = '\0';
        vptr = p + 1;
        nptr = buf;
    } else {
        vptr = value;
        nptr = name;
    }

    if (!section) {
        p = strrchr(nptr, '.');
        if (!p)
            goto invalid;
        len = p - nptr;
        if (name) {
            if (len >= sizeof(buf))
                goto invalid;
            memcpy(buf, nptr, len);
        }
        buf[len] = '\0';
        nptr = p + 1;
        sptr = buf;
    } else {
        sptr = section;
    }

    r = sg_configfile_set(&sg_cvar_arg, sptr, nptr, vptr);
    if (r) { }
    return;

invalid:
    return;
}

void
sg_cvar_sets(const char *section, const char *name, const char *value)
{
    int r;
    r = sg_configfile_set(&sg_cvar_conf, section, name, value);
    if (r) { }
    sg_configfile_remove(&sg_cvar_arg, section, name);
}

int
sg_cvar_gets(const char *section, const char *name, const char **value)
{
    const char *r;
    r = sg_configfile_get(&sg_cvar_arg, section, name);
    if (!r)
        r = sg_configfile_get(&sg_cvar_conf, section, name);
    if (!r)
        return 0;
    *value = r;
    return 1;
}

void
sg_cvar_seti(const char *section, const char *name, int value)
{
    sg_cvar_setl(section, name, value);
}

int
sg_cvar_geti(const char *section, const char *name, int *value)
{
    long l;
    if (!sg_cvar_getl(section, name, &l))
        return 0;
    if (l >= INT_MAX)
        *value = INT_MAX;
    else if (l <= INT_MIN)
        *value = INT_MIN;
    else
        *value = l;
    return 1;
}

void
sg_cvar_setl(const char *section, const char *name, long value)
{
    char buf[32];
#if !defined(_WIN32)
    snprintf(buf, sizeof(buf), "%ld", value);
#else
    _snprintf_s(buf, sizeof(buf), _TRUNCATE, "%ld", value);
#endif
    sg_cvar_sets(section, name, buf);
}

int
sg_cvar_getl(const char *section, const char *name, long *value)
{
    char *e;
    char const *s;
    long l;

    if (!sg_cvar_gets(section, name, &s))
        return 0;
    if (!*s)
        return 0;
    l = strtol(s, &e, 0);
    if (*e)
        return 0;
    *value = l;
    return 1;
}

void
sg_cvar_setb(const char *section, const char *name, int value)
{
    sg_cvar_sets(section, name, value ? "yes" : "no");
}

/* Recognizes true/false, yes/no, on/off */
int
sg_cvar_getb(const char *section, const char *name, int *value)
{
    const char *s;
    size_t l;
    unsigned i, b;
    char buf[8];

    if (!sg_cvar_gets(section, name, &s))
        return 0;
    l = strlen(s);
    if (l > 5)
        return 0;
    for (i = 0; i < l; ++i)
        buf[i] = to_lower(s[i]);
    buf[l] = '\0';

    b = 2;
    switch (l) {
    case 2:
        if (!strcmp(buf, "no"))
            b = 0;
        else if (!strcmp(buf, "on"))
            b = 1;
        break;

    case 3:
        if (!strcmp(buf, "yes"))
            b = 1;
        else if (!strcmp(buf, "off"))
            b = 0;
        break;

    case 4:
        if (!strcmp(buf, "true"))
            b = 1;
        break;

    case 5:
        if (!strcmp(buf, "false"))
            b = 0;
        break;

    default:
        break;
    }

    if (b == 2)
        return 0;
    *value = b;
    return 1;
}

void
sg_cvar_setd(const char *section, const char *name, double value)
{
    char buf[64];
    if (isfinite(value)) {
#if !defined(_WIN32)
        snprintf(buf, sizeof(buf), "%.17g", value);
#else
        _snprintf(buf, sizeof(buf), "%.17g", value);
        buf[sizeof(buf) - 1] = 0;
#endif
        sg_cvar_sets(section, name, buf);
    } else if (isnan(value)) {
        sg_cvar_sets(section, name, "nan");
    } else {
        sg_cvar_sets(section, name, value > 0 ? "+inf" : "-inf");
    }
}

int
sg_cvar_getd(const char *section, const char *name, double *value)
{
    const char *s;
    char *p;
    double d;

    if (!sg_cvar_gets(section, name, &s))
        return 0;
    if (!*s)
        return 0;

    d = strtod(s, &p);
    if (*p)
        return 0;
    *value = d;
    return 1;
}
