#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "error.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void
sg_error_clobber(const struct sg_error_domain *dom,
                 long code, const char *msg)
{
    struct sg_logger *logger = sg_logger_get(NULL);
    if (code)
        sg_logf(logger, LOG_ERROR,
                "warning: error discarded: %s (%s %ld)\n",
                msg, dom->name, code);
    else
        sg_logf(logger, LOG_ERROR,
                "warning: error discarded: %s (%s)\n",
                msg, dom->name);
}

void
sg_error_setf(struct sg_error **err, const struct sg_error_domain *dom,
              long code, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    sg_error_setv(err, dom, code, fmt, ap);
    va_end(ap);
}

void
sg_error_setv(struct sg_error **err, const struct sg_error_domain *dom,
              long code, const char *fmt, va_list ap)
{
    char buf[512];
#if !defined(_WIN32)
    vsnprintf(buf, sizeof(buf), fmt, ap);
#else
    _vsnprintf(buf, sizeof(buf), fmt, ap);
    buf[sizeof(buf) - 1] = '\0';
#endif
    sg_error_sets(err, dom, code, buf);
}

void
sg_error_sets(struct sg_error **err, const struct sg_error_domain *dom,
              long code, const char *msg)
{
    struct sg_error *e;
    size_t len;
    if (!err || *err) {
        sg_error_clobber(dom, code, msg);
        return;
    }
    if (msg) {
        len = strlen(msg);
        e = malloc(sizeof(struct sg_error) + len + 1);
        if (!e) {
            fputs("error: out of memory in error handler\n", stderr);
            abort();
        }
        memcpy(e + 1, msg, len + 1);
        e->msg = (char *) (e + 1);
    } else {
        e = malloc(sizeof(struct sg_error));
        e->msg = NULL;
    }
    e->refcount = 1;
    e->domain = dom;
    e->code = code;
    *err = e;
}

void
sg_error_move(struct sg_error **dest, struct sg_error **src)
{
    struct sg_error *d = *dest, *s = *src;
    if (d) {
        if (s) {
            sg_error_clobber(s->domain, s->code, s->msg);
            if (--s->refcount == 0)
                free(s);
            *src = NULL;
        }
    } else {
        *dest = s;
        *src = NULL;
    }
}

void
sg_error_clear(struct sg_error **err)
{
    struct sg_error *e = *err;
    if (e && --e->refcount == 0) {
        free(e);
        *err = NULL;
    }
}

const struct sg_error_domain SG_ERROR_INVALPATH = { "invalpath" };
const struct sg_error_domain SG_ERROR_NOTFOUND = { "notfound" };
const struct sg_error_domain SG_ERROR_NOMEM = { "nomem" };
const struct sg_error_domain SG_ERROR_DATA = { "data" };

void
sg_error_notfound(struct sg_error **err, const char *path)
{
    sg_error_setf(err, &SG_ERROR_NOTFOUND, 0,
                  "file not found: '%s'", path);
}

void
sg_error_nomem(struct sg_error **err)
{
    sg_error_sets(err, &SG_ERROR_NOMEM, 0, "out of memory");
}

void
sg_error_data(struct sg_error **err, const char *fmtname)
{
    sg_error_setf(err, &SG_ERROR_DATA, 0, "corrupt %s file", fmtname);
}

#ifdef _WIN32
#define COBJMACROS 1
#include <WS2tcpip.h>
#include <Windows.h>
//#include <OAIdl.h>

extern const struct sg_error_domain SG_ERROR_WINDOWS = { "windows" };

static void
sg_error_setw(struct sg_error **err, const struct sg_error_domain *dom,
              long code, wchar_t *wtext, int wlen)
{
    struct sg_error *e = NULL;
    int alen, r;
    char *atext;

    alen = WideCharToMultiByte(CP_UTF8, 0, wtext, wlen, NULL, 0, NULL, NULL);
    if (!alen)
        goto error;
    e = malloc(sizeof(*e) + alen + 1);
    if (!e)
        goto error;
    atext = (char *) (e + 1);
    r = WideCharToMultiByte(CP_UTF8, 0, wtext, wlen, atext, alen, NULL, NULL);
    if (!r)
        goto error;
    atext[alen] = '\0';

    if (!err || *err) {
        sg_error_clobber(dom, code, atext);
    } else {
        e->refcount = 1;
        e->domain = dom;
        e->msg = atext;
        e->code = code;
        *err = e;
    }
    return;

error:
    fputs("error: out of memory in error handler\n", stderr);
    abort();
}

void
sg_error_win32(struct sg_error **err, unsigned long code)
{
    LPWSTR wtext;
    int len;

    len = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        code,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPWSTR) &wtext,
        0, NULL);
    if (!len) {
        sg_error_sets(err, &SG_ERROR_WINDOWS, code, "<error in error handler>");
    } else {
        sg_error_setw(err, &SG_ERROR_WINDOWS, code, wtext, len);
        LocalFree(wtext);
    }
}

/*
    Yes, this is a pain.
    http://stackoverflow.com/questions/4597932/how-can-i-is-there-a-way-to-convert-an-hresult-into-a-system-specific-error-me
    http://stackoverflow.com/questions/455434/how-should-i-use-formatmessage-properly-in-c
*/
void
sg_error_hresult(struct sg_error **err, long code)
{
    IErrorInfo *iei;
    HRESULT hr;
    BSTR bstr;
    WORD facility;

    facility = HRESULT_FACILITY(code);
    hr = GetErrorInfo(0, &iei);
    if (SUCCEEDED(hr) && iei) {
        IErrorInfo_GetDescription(iei, &bstr);
        sg_error_setw(err, &SG_ERROR_WINDOWS, code, bstr + 1, *bstr);
        SysFreeString(bstr);
        IUnknown_Release(iei);
        return;
    }
    if (facility == FACILITY_ITF) {
        sg_error_sets(err, &SG_ERROR_WINDOWS, code, "unknown HRESULT (FACILITY_ITF)");
        return;
    }
    sg_error_win32(err, code);
}

#else

const struct sg_error_domain SG_ERROR_ERRNO = { "errno" };

#include <errno.h>
#if 0 && defined(__GLIBC__)

void
sg_error_errno(struct sg_error **err, int code)
{
    char buf[256], *r;
    if (code == ENOMEM) {
        sg_error_nomem(err);
    } else {
        r = strerror_r(code, buf, sizeof(buf));
        error_sets(err, &SG_ERROR_ERRNO, code, r);
    }
}

#else

void
sg_error_errno(struct sg_error **err, int code)
{
    char buf[256];
    int r;
    if (code == ENOMEM) {
        sg_error_nomem(err);
    } else {
        r = strerror_r(code, buf, sizeof(buf));
        sg_error_sets(err, &SG_ERROR_ERRNO, code, r ? NULL : buf);
    }
}

#endif

#endif

#if !defined(_WIN32)
#include <netdb.h>
#endif

const struct sg_error_domain SG_ERROR_GETADDRINFO = { "getaddrinfo" };

void
sg_error_gai(struct sg_error **err, int code)
{
    const char *desc = gai_strerror(code);
    sg_error_sets(err, &SG_ERROR_GETADDRINFO, code, desc);
}
