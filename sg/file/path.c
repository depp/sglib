/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "sg/defs.h"
#if defined(_WIN32)
#include <Windows.h>
#endif

#define SG_PATH_INITSZ 1

#include "impl.h"
#include "sg/cvar.h"
#include "sg/error.h"
#include "sg/file.h"
#include "sg/log.h"
#include "../private.h"
#include <stdlib.h>
#include <string.h>

struct sg_path {
    pchar *path;
    size_t len;
};

struct sg_paths {
    struct sg_path *a;
    unsigned wcount, acount;
    unsigned wmaxlen, amaxlen;
    unsigned alloc;
};

static struct sg_paths sg_paths;

/* Copy the normalized path 'src' to the destination 'dst', converting
   normalized directory separators to platform directory separators
   (e.g., turn backslashes backwards on Windows).  */
static void
sg_path_copy(pchar *dest, const char *src, size_t len)
{
#if defined(_WIN32)
    size_t i;
    for (i = 0; i < len; ++i)
        dest[i] = src[i] == '/' ? SG_PATH_DIRSEP : src[i];
#else
    memcpy(dest, src, len);
#endif
}

/* Same, without separator normalization.  */
static void
sg_path_copy2(pchar *dest, const char *src, size_t len)
{
#if defined(_WIN32)
    size_t i;
    for (i = 0; i < len; ++i)
        dest[i] = src[i];
#else
    memcpy(dest, src, len);
#endif
}

#define MAX_EXTENSIONS 8

struct sg_file_ext {
    const char *p;
    unsigned len;
};

struct sg_file *
sg_file_open(const char *path, size_t pathlen, int flags,
             const char *extensions, struct sg_error **e)
{
    struct sg_file_ext exts[MAX_EXTENSIONS];
    struct sg_file *f;
    struct sg_path *a;
    int nlen, r, stripped;
    unsigned i, j, pcount, pmaxlen, nexts, extlen, emaxlen;
    char nbuf[SG_MAX_PATH];
    pchar *pbuf = NULL, *p;
    const char *extp, *extq;

    if (flags & SG_WRONLY) {
        flags |= SG_WRITABLE;
        extensions = NULL;
    }

    nlen = sg_path_norm(nbuf, path, pathlen, e);
    if (nlen < 0)
        return NULL;

    extp = extensions;
    nexts = 0;
    stripped = 0;
    emaxlen = 0;
    while (extp) {
        extq = extp;
        extp = strchr(extp, ':');
        if (extp) {
            extlen = (unsigned) (extp - extq);
            extp++;
        } else {
            extlen = (unsigned) strlen(extq);
        }
        if (nexts == MAX_EXTENSIONS) {
            sg_logs(sg_logger_get(NULL), LOG_ERROR,
                    "list of extensions is too long");
            break;
        }
        if (extlen > emaxlen)
            emaxlen = extlen;
        if (!stripped &&
            (size_t) nlen > extlen + 1 &&
            nbuf[nlen - extlen - 1] == '.' &&
            !memcmp(nbuf + nlen - extlen, extq, extlen))
        {
            if (nexts)
                memcpy(&exts[nexts], &exts[0], sizeof(*exts));
            exts[0].p = extq;
            exts[0].len = extlen;
            nlen -= extlen + 1;
            stripped = 1;
        } else {
            exts[nexts].p = extq;
            exts[nexts].len = extlen;
        }
        nexts++;
    }
    if (!nexts) {
        exts[0].p = NULL;
        exts[0].len = 0;
        nexts = 1;
    }
    if (emaxlen)
        emaxlen += 1;
    if (nlen + emaxlen >= SG_MAX_PATH) {
        sg_error_sets(e, &SG_ERROR_INVALPATH, 0,
                      "path too long for given extension list");
        return NULL;
    }

    a = sg_paths.a;
    if (flags & SG_LOCAL) {
        pcount = sg_paths.wcount;
        pmaxlen = sg_paths.wmaxlen;
    } else {
        pcount = sg_paths.acount;
        pmaxlen = sg_paths.amaxlen;
    }
    if (!pcount)
        goto notfound;
    pbuf = malloc((pmaxlen + nlen + emaxlen + 1) * sizeof(pchar));
    if (!pbuf)
        goto nomem;
    sg_path_copy(pbuf + pmaxlen, nbuf, nlen);
    for (i = 0; i < pcount; ++i) {
        p = pbuf + pmaxlen - a[i].len;
        memcpy(p, a[i].path, a[i].len * sizeof(pchar));
        for (j = 0; j < nexts; ++j) {
            extp = exts[j].p;
            extlen = exts[j].len;
            if (extlen) {
                pbuf[pmaxlen + nlen] = '.';
                sg_path_copy2(pbuf + pmaxlen + nlen + 1, extp, extlen);
                pbuf[pmaxlen + nlen + extlen + 1] = '\0';
            } else {
                pbuf[pmaxlen + nlen] = '\0';
            }
            r = sg_file_tryopen(&f, p, flags, e);
            if (r > 0)
                goto done;
            if (r < 0) {
                f = NULL;
                goto done;
            }
        }
    }
    goto notfound;

notfound:
    sg_error_notfound(e, nbuf);
    f = NULL;
    goto done;

nomem:
    sg_error_nomem(e);
    f = NULL;
    goto done;

done:
    free(pbuf);
    return f;
}

/* Add a path to the given search paths.  The path should be malloc'd,
   NUL-terminated, and must end with the directory separator.  This
   function assumes ownership of the path pointer, either freeing it
   or using it.  */

static void
sg_path_add(struct sg_paths *p, pchar *path, int writable)
{
    struct sg_path *na;
    size_t pathlen;
    unsigned nalloc, pos;
    int r;

    if (!writable) {
        r = sg_path_checkdir(path);
        if (!r) {
            free(path);
            return;
        }
    }

#if defined(_WIN32)
    pathlen = wcslen(path);
#else
    pathlen = strlen(path);
#endif

    if (p->acount >= p->alloc) {
        nalloc = p->alloc ? p->alloc * 2 : SG_PATH_INITSZ;
        na = realloc(p->a, sizeof(*na) * nalloc);
        if (!na) goto nomem;
        p->a = na;
        p->alloc = nalloc;
    }
    pos = p->acount;
    if (pathlen > p->amaxlen)
        p->amaxlen = (unsigned) pathlen;
    p->acount += 1;
    if (writable) {
        pos = p->wcount;
        if (pathlen > p->wmaxlen)
            p->wmaxlen = (unsigned) pathlen;
        p->wcount += 1;
    }
    memmove(p->a + pos + 1, p->a + pos,
            sizeof(*p->a) * (p->acount - pos - 1));
    p->a[pos].path = path;
    p->a[pos].len = pathlen;
    return;

nomem:
    abort();
    return;
}

#if PATH_MAX > 0
#define PATH_BUFSZ PATH_MAX
#else
#define PATH_BUFSZ 1024
#endif

static const struct {
    char dirname[5];
    char varname[5];
} sg_path_defaults[2] = {
    { "User", "user" },
    { "Data", "data" }
};

void
sg_path_init(void)
{
    const char *paths, *p, *start, *end;
    pchar buf[PATH_BUFSZ], *str = NULL, *q;
    int r, i, writable;
    size_t elen = 0, dlen, slen;

    for (i = 0; i < 2; ++i) {
        writable = !i;
        r = sg_cvar_gets("path", sg_path_defaults[i].varname, &paths);
        if (r && *paths) {
            /* Use a path from the config settings */

            p = paths;
            do {
                start = p;
                end = strchr(p, SG_PATH_PATHSEP);
                if (end) {
                    p = end + 1;
                } else {
                    end = p + strlen(p);
                    p = end;
                }
                if (start != end) {
                    dlen = end - start;
#if defined(_WIN32)
                    r = MultiByteToWideChar(
                        CP_UTF8, 0, start, (int) dlen, NULL, 0);
                    if (!r) goto fail;
                    slen = r;
                    str = malloc(sizeof(wchar_t) * (slen + 2));
                    if (!str) goto fail;
                    r = MultiByteToWideChar(
                        CP_UTF8, 0, start, (int) dlen, str, (int) slen);
                    if (!r) goto fail;
#else
                    slen = dlen;
                    str = malloc(dlen + 2);
                    if (!str) goto fail;
                    memcpy(str, start, dlen);
#endif
                    if (str[slen-1] != SG_PATH_DIRSEP)
                        str[slen++] = SG_PATH_DIRSEP;
                    str[slen] = '\0';
                    sg_path_add(&sg_paths, str, writable);
                    str = NULL;
                }
            } while (*end);
        } else {
            /* Use a default EXE-relative path */

            if (!elen) {
                r = sg_path_getexepath(buf, PATH_BUFSZ);
                if (!r) goto fail;
                q = pathrchr(buf, SG_PATH_DIRSEP);
                if (!q) goto fail;
                q++;
                elen = q - buf;
            }

            dlen = strlen(sg_path_defaults[i].dirname);
            str = malloc(sizeof(pchar) * (elen + dlen + 2));
            if (!str) goto fail;
            memcpy(str, buf, sizeof(pchar) * elen);
            sg_path_copy2(str + elen, sg_path_defaults[i].dirname, dlen);
            str[elen+dlen] = SG_PATH_DIRSEP;
            str[elen+dlen+1] = '\0';

            sg_path_add(&sg_paths, str, writable);
            str = NULL;
        }
    }

    return;

fail:
    abort();
}
