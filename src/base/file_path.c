#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define SG_PATH_INITSZ 1

#include "cvar.h"
#include "error.h"
#include "file.h"
#include "file_impl.h"
#include "log.h"
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
#if SG_PATH_DIRSEP != '/'
    size_t i;
    for (i = 0; i < len; ++i)
        dest[i] = src[i] == '/' ? SG_PATH_DIRSEP : src[i];
#elif defined(SG_PATH_UNICODE)
    size_t i;
    for (i = 0; i < len; ++i)
        dest[i] = src[i];
#else
    memcpy(dest, src, len);
#endif
}   

/* Same, without separator normalization.  */
static void
sg_path_copy2(pchar *dest, const char *src, size_t len)
{
#if defined(SG_PATH_UNICODE)
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

    if (flags & SG_WRONLY)
        flags |= SG_WRITABLE;

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

/* Add a path to the given search paths.  Return >0 on success, 0 if
   the path is discarded, and <0 if some other error occurred.  */
static int
sg_path_add(struct sg_paths *p, const char *path, size_t len, int flags)
{
    struct sg_path npath, *na;
    unsigned nalloc, pos;
    int r;

    r = sg_path_getdir(&npath.path, &npath.len, path, len, flags);
    if (r <= 0)
        return r;

    if (p->acount >= p->alloc) {
        nalloc = p->alloc ? p->alloc * 2 : SG_PATH_INITSZ;
        na = realloc(p->a, sizeof(*na) * nalloc);
        if (!na) {
            free(npath.path);
            goto nomem;
        }
        p->a = na;
        p->alloc = nalloc;
    }
    pos = p->acount;
    if (npath.len > p->amaxlen)
        p->amaxlen = (unsigned) npath.len;
    p->acount += 1;
    if (flags & SG_PATH_WRITABLE) {
        pos = p->wcount;
        if (npath.len > p->wmaxlen)
            p->wmaxlen = (unsigned) npath.len;
        p->wcount += 1;
    }
    memmove(p->a + pos + 1, p->a + pos,
            sizeof(*p->a) * (p->acount - pos - 1));
    memcpy(p->a + pos, &npath, sizeof(npath));
    return 1;

nomem:
    abort();
    return -1;
}

void
sg_path_init(void)
{
    const char *p, *sep;
    int r;

    r = sg_cvar_gets("path", "data-dir", &p);
    if (!r)
        p = "data";

    sg_path_add(&sg_paths, p, strlen(p), SG_PATH_EXEDIR);

    r = sg_cvar_gets("path", "data-path", &p);
    if (!r)
        p = "";
    while (*p) {
        sep = strchr(p, SG_PATH_PATHSEP);
        if (!sep) {
            if (*p)
                sg_path_add(&sg_paths, p, strlen(p), 0);
            break;
        }
        if (p != sep)
            sg_path_add(&sg_paths, p, sep - p, 0);
        p = sep + 1;
    }
}
