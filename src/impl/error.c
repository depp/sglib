#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "error.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void
error_clobber(const struct error_domain *dom, long code, const char *msg)
{
    if (code)
        fprintf(stderr, "warning: error discarded: %s (%s %ld)\n",
                msg, dom->name, code);
    else
        fprintf(stderr, "warning: error discarded: %s (%s)\n",
                msg, dom->name);
}

void
error_set(struct error **err, const struct error_domain *dom,
          long code, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    error_setv(err, dom, code, fmt, ap);
    va_end(ap);
}

void
error_setv(struct error **err, const struct error_domain *dom,
           long code, const char *fmt, va_list ap)
{
    char buf[512];
    int r;
#if !defined(_WIN32)
    r = vsnprintf(buf, sizeof(buf), fmt, ap);
#else
    r = _vsnprintf(buf, sizeof(buf), fmt, ap);
    buf[sizeof(buf) - 1] = '\0';
#endif
    error_sets(err, dom, code, buf);
}

void
error_sets(struct error **err, const struct error_domain *dom,
           long code, const char *msg)
{
    struct error *e;
    size_t len;
    if (!err || *err) {
        error_clobber(dom, code, msg);
        return;
    }
    if (msg) {
        len = strlen(msg);
        e = malloc(sizeof(struct error) + len + 1);
        if (!e) {
            fputs("error: out of memory in error handler\n", stderr);
            abort();
        }
        memcpy(e + 1, msg, len + 1);
        e->msg = (char *) (e + 1);
    } else {
        e = malloc(sizeof(struct error));
        e->msg = NULL;
    }
    e->domain = dom;
    e->code = code;
}

void
error_move(struct error **dest, struct error **src)
{
    struct error *d = *dest, *s = *src;
    if (d) {
        if (s) {
            error_clobber(s->domain, s->code, s->msg);
            free(s);
            *src = NULL;
        }
    } else {
        *dest = s;
        *src = NULL;
    }
}

void
error_clear(struct error **err)
{
    struct error *e = *err;
    if (e) {
        free(e);
        *err = NULL;
    }
}

const struct error_domain ERROR_NOTFOUND = { "notfound" };

const struct error_domain ERROR_NOMEM = { "nomem" };

void
error_set_notfound(struct error **err, const char *path)
{
    error_set(err, &ERROR_NOTFOUND, 0,
              "file not found: '%s'", path);
}

void
error_set_nomem(struct error **err)
{
    error_sets(err, &ERROR_NOMEM, 0, NULL);
}

const struct error_domain ERROR_ERRNO = { "errno" };

#if 0 && defined(__GLIBC__)

void
error_errno(struct error **err, int code)
{
    char buf[256], *r;
    r = strerror_r(code, buf, sizeof(buf));
    error_sets(err, &ERROR_ERRNO, code, r);
}

#else

void
error_errno(struct error **err, int code)
{
    char buf[256];
    int r;
    r = strerror_r(code, buf, sizeof(buf));
    error_sets(err, &ERROR_ERRNO, code, r ? NULL : buf);
}

#endif
