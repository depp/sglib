#include "cvar.h"
#include "configfile.h"

#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct configfile sg_cvar_conf, sg_cvar_arg;

static int
to_lower(int c)
{
    if (c >= 'A' && c <= 'Z')
        return c + ('a' - 'A');
    return c;
}

void
sg_cvar_addarg(const char *section, const char *name, const char *v)
{
    char buf[128];
    const char *p, *sptr, *nptr, *vptr;
    size_t len;
    int r;

    if (!name) {
        p = strchr(v, '=');
        if (!p)
            goto invalid;
        len = p - v;
        if (len >= sizeof(buf))
            goto invalid;
        memcpy(buf, v, len);
        buf[len] = '\0';
        vptr = p + 1;
        nptr = buf;
    } else {
        vptr = v;
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

    r = configfile_set(&sg_cvar_arg, sptr, nptr, vptr);
    if (r) { }
    return;

invalid:
    return;
}

/*
void
sg_cvar_load(void)
{
    configfile_destroy(&sg_cvar_conf);
    configfile_init(&sg_cvar_conf);
    configfile_read(...);
}
*/

void
sg_cvar_sets(const char *section, const char *name, const char *v)
{
    int r;
    r = configfile_set(&sg_cvar_conf, section, name, v);
    if (r) { }
    configfile_remove(&sg_cvar_arg, section, name);
}

int
sg_cvar_gets(const char *section, const char *name, const char **v)
{
    const char *r;
    r = configfile_get(&sg_cvar_arg, section, name);
    if (!r)
        r = configfile_get(&sg_cvar_conf, section, name);
    if (!r)
        return 0;
    *v = r;
    return 1;
}

void
sg_cvar_seti(const char *section, const char *name, int v)
{
    sg_cvar_setl(section, name, v);
}

int
sg_cvar_geti(const char *section, const char *name, int *v)
{
    long l;
    if (!sg_cvar_getl(section, name, &l))
        return 0;
    if (l >= INT_MAX)
        *v = INT_MAX;
    else if (l <= INT_MIN)
        *v = INT_MIN;
    else
        *v = l;
    return 1;
}

void
sg_cvar_setl(const char *section, const char *name, long v)
{
    char buf[32];
#if !defined(_WIN32)
    snprintf(buf, sizeof(buf), "%ld", v);
#else
    _snprintf(buf, sizeof(buf), "%ld", v);
    buf[sizeof(buf) - 1] = '\0';
#endif
    sg_cvar_sets(section, name, buf);
}

int
sg_cvar_getl(const char *section, const char *name, long *v)
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
    *v = l;
    return 1;
}

void
sg_cvar_setb(const char *section, const char *name, int v)
{
    sg_cvar_sets(section, name, v ? "yes" : "no");
}

/* Recognizes true/false, yes/no, on/off */
int
sg_cvar_getb(const char *section, const char *name, int *v)
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
    *v = b;
    return 1;
}

void
sg_cvar_setd(const char *section, const char *name, double v)
{
    char buf[64];
    if (isfinite(v)) {
#if !defined(_WIN32)
        snprintf(buf, sizeof(buf), "%.17g", v);
#else
        _snprintf(buf, sizeof(buf), "%.17g", v);
        buf[sizeof(buf) - 1] = 0;
#endif
        sg_cvar_sets(section, name, buf);
    } else if (isinf(v)) {
        sg_cvar_sets(section, name, v > 0 ? "+inf" : "-inf");
    } else {
        sg_cvar_sets(section, name, "nan");
    }
}

int
sg_cvar_getd(const char *section, const char *name, double *v)
{
    const char *s;
    size_t l;
    unsigned i, neg;
    char buf[8], *p;
    double d;

    if (!sg_cvar_gets(section, name, &s))
        return 0;
    if (!*s)
        return 0;

    l = strlen(s);
    if (l <= 4) {
        for (i = 0; i < l; ++i)
            buf[i] = to_lower(s[i]);
        buf[l] = '\0';
        p = buf;
        if (*p == '+') {
            p++;
            l--;
            neg = 0;
        } else if (*p == '-') {
            p++;
            l--;
            neg = 1;
        } else {
            neg = 0;
        }
        if (!strcmp(p, "nan")) {
            *v = NAN;
            return 1;
        }
        if (!strcmp(p, "inf")) {
            *v = neg ? -INFINITY : INFINITY;
            return 1;
        }
    }

    d = strtod(s, &p);
    if (*p)
        return 0;
    *v = d;
    return 1;
}
