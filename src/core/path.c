/* Copyright 2012-2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "sg/defs.h"
#if defined _WIN32
#include <Windows.h>
#endif

#include "file_impl.h"
#include "sg/cvar.h"
#include "sg/entry.h"
#include "sg/error.h"
#include "sg/file.h"
#include "sg/log.h"
#include "private.h"
#include <stdlib.h>
#include <string.h>

#define SG_PATH_MAXEXTS 8
#define SG_PATH_MAXCOUNT 8

#if PATH_MAX > 0
#define SG_PATH_BUFSZ PATH_MAX
#else
#define SG_PATH_BUFSZ 1024
#endif

struct sg_path {
    pchar *path;
    size_t len;
};

struct sg_paths {
    struct sg_path *path;
    unsigned pathcount;
    unsigned maxlen;
    struct sg_cvar_string cvar[2];
};

struct sg_file_ext {
    const char *p;
    unsigned len;
};

static struct sg_paths sg_paths;

SG_ATTR_NORETURN
static void
sg_path_initabort(void)
{
    sg_sys_abort("Failed to initialize paths.");
}

/* Copy the normalized path 'src' to the destination 'dst', converting
   normalized directory separators to platform directory separators
   (e.g., turn slashes backwards on Windows).  */
static void
sg_path_copy(pchar *dest, const char *src, size_t len);

#if defined _WIN32

static void
sg_path_copy(wchar_t *dest, const char *src, size_t len)
{
    size_t i;
    for (i = 0; i < len; ++i)
        dest[i] = src[i] == '/' ? SG_PATH_DIRSEP : src[i];
}

#else

static void
sg_path_copy(char *dest, const char *src, size_t len)
{
    memcpy(dest, src, len);
}

#endif

pchar *
sg_file_createpath(const char *path, size_t pathlen,
                   struct sg_error **err)
{
    int r;
    pchar *result;

    {
        size_t plen = sg_paths.path[0].len;
        char nbuf[SG_MAX_PATH];
        int nlen;
        nlen = sg_path_norm(nbuf, path, pathlen, err);
        if (nlen < 0)
            return NULL;
        result = malloc(sizeof(pchar) * (plen + nlen + 1));
        if (!result) {
            sg_error_nomem(err);
            return NULL;
        }
        pmemcpy(result, sg_paths.path[0].path, plen);
        sg_path_copy(result + plen, nbuf, nlen);
        result[plen + nlen] = 0;
    }

    r = sg_path_mkpardir(result, err);
    if (r) {
        free(result);
        return NULL;
    }

    return result;
}

struct sg_file *
sg_file_open(const char *path, size_t pathlen, int flags,
             const char *extensions, struct sg_error **err)
{
    struct sg_file *fp;
    char nbuf[SG_MAX_PATH];
    pchar *pbuf, *pptr;
    struct sg_path *search;
    unsigned sflags, searchcount, maxslen;
    int nlen;

    sflags = flags & (SG_USERONLY | SG_DATAONLY);
    search = sg_paths.path;
    searchcount = sg_paths.pathcount;
    maxslen = sg_paths.maxlen;
    if (flags & SG_WRONLY) {
        if (sflags != 0)
            goto badflags;
        searchcount = 1;
        if (extensions) {
            sg_error_invalid(err, __FUNCTION__, "extensions");
            return NULL;
        }
    } else {
        switch (sflags) {
        case 0:
            break;
        case SG_USERONLY:
            searchcount = 1;
            break;
        case SG_DATAONLY:
            search++;
            searchcount--;
            break;
        default:
            goto badflags;
        }
    }
    if (searchcount == 1)
        maxslen = search[0].len;

    nlen = sg_path_norm(nbuf, path, pathlen, err);
    if (nlen < 0)
        return NULL;

    if (extensions) {
        struct sg_file_ext exts[SG_PATH_MAXEXTS];
        const char *extp = extensions, *ext, *extsep;
        unsigned extcount = 0, maxelen = 0, extlen, i, j, k;
        int r;

        while (extp) {
            ext = extp;
            extsep = strchr(extp, ':');
            if (!extsep) {
                extp = NULL;
                extlen = (unsigned) strlen(ext);
            } else {
                extp = extsep + 1;
                extlen = (unsigned) (extsep - ext);
            }
            if (!extlen)
                continue;
            if (extcount >= SG_PATH_MAXEXTS) {
                sg_logs(SG_LOG_ERROR, "List of extensions is too long.");
                break;
            }
            if (extlen > maxelen)
                maxelen = extlen;
            exts[extcount].p = ext;
            exts[extcount].len = extlen;
            extcount++;
        }
        /* If extensions is not NULL but lists no extensions, it could
           be caused by support for all file formats being compiled
           out, in which case "file not found" is the most sensible
           result, because there is no satisfactory file which could
           be found.  */
        if (!extcount)
            goto notfound;
        maxelen += 1;
        if (nlen + maxelen >= SG_MAX_PATH) {
            sg_error_sets(err, &SG_ERROR_INVALPATH, 0,
                          "path too long for given extension list");
            return NULL;
        }

        if (!searchcount)
            goto notfound;

        pbuf = malloc((maxslen + nlen + maxelen + 1) * sizeof(pchar));
        if (!pbuf)
            goto nomem;
        sg_path_copy(pbuf + maxslen, nbuf, nlen);
        pbuf[maxslen + nlen] = '.';
        for (i = 0; i < searchcount; i++) {
            pptr = pbuf + maxslen - search[i].len;
            pmemcpy(pptr, search[i].path, search[i].len);
            for (j = 0; j < extcount; j++) {
                ext = exts[j].p;
                extlen = exts[j].len;
                for (k = 0; k < extlen; k++)
                    pbuf[maxslen + nlen + 1 + k] = ext[k];
                pbuf[maxslen + nlen + 1 + extlen] = '\0';
                r = sg_file_tryopen(&fp, pbuf, flags, err);
                if (r != 0)
                    goto done;
            }
        }
    } else {
        unsigned i;
        int r;

        if (!searchcount)
            goto notfound;

        pbuf = malloc((maxslen + nlen + 1) * sizeof(pchar));
        if (!pbuf)
            goto nomem;
        sg_path_copy(pbuf + maxslen, nbuf, nlen);
        pbuf[maxslen + nlen] = '\0';
        for (i = 0; i < searchcount; i++) {
            pptr = pbuf + maxslen - search[i].len;
            pmemcpy(pptr, search[i].path, search[i].len);
            r = sg_file_tryopen(&fp, pbuf, flags, err);
            if (r != 0)
                goto done;
        }
    }

    goto notfound;

notfound:
    sg_error_notfound(err, nbuf);
    fp = NULL;
    goto done;

nomem:
    sg_error_nomem(err);
    fp = NULL;
    goto done;

badflags:
    sg_error_invalid(err, __FUNCTION__, "flags");
    fp = NULL;
    goto done;

done:
    free(pbuf);
    return fp;
}

static void
sg_path_init3(struct sg_path *cvar)
{
    struct sg_path paths[SG_PATH_MAXCOUNT], *gpaths;
    pchar *userbuf = NULL, *databuf = NULL, *pathptr;
    size_t len, totallen, maxlen, count, i;

    if (cvar[0].len > 0) {
        paths[0] = cvar[0];
    } else {
        userbuf = malloc(sizeof(*userbuf) * SG_PATH_BUFSZ);
        if (!userbuf)
            goto error;
        len = sg_path_getuserpath(userbuf, SG_PATH_BUFSZ);
        if (!len)
            goto error;
        paths[0].path = userbuf;
        paths[0].len = len;
    }

    if (cvar[1].len > 0) {
        pchar *pp, *pe, *ps;
        pp = cvar[1].path;
        pe = pp + cvar[1].len;
        count = 1;
        while (1) {
            ps = pmemchr(pp, SG_PATH_PATHSEP, pe - pp);
            if (!ps)
                ps = pe;
            if (pp != ps) {
                if (count >= SG_PATH_MAXCOUNT) {
                    sg_logs(SG_LOG_WARN, "Too many paths");
                    break;
                }
                paths[count].path = pp;
                paths[count].len = ps - pp;
                count++;
            }
            if (ps == pe)
                break;
            pp = ps + 1;
        }
    } else {
        databuf = malloc(sizeof(*databuf) * SG_PATH_BUFSZ);
        if (!databuf)
            goto error;
        len = sg_path_getdatapath(databuf, SG_PATH_BUFSZ);
        if (!len)
            goto error;
        paths[1].path = databuf;
        paths[1].len = len;
        count = 2;
    }

    totallen = 0;
    maxlen = 0;
    for (i = 0; i < count; i++) {
        len = paths[i].len;
        if (paths[i].path[len - 1] != SG_PATH_DIRSEP)
            len++;
        totallen += len;
        if (len > maxlen)
            maxlen = len;
    }

    gpaths = malloc(sizeof(*gpaths) * count +
                    sizeof(pchar) * (totallen + count));
    if (!gpaths)
        goto error;
    pathptr = (pchar *) (gpaths + count);
    for (i = 0; i < count; i++) {
        len = paths[i].len;
        pmemcpy(pathptr, paths[i].path, len);
        if (pathptr[len - 1] != SG_PATH_DIRSEP) {
            pathptr[len] = SG_PATH_DIRSEP;
            len++;
        }
        pathptr[len] = '\0';
        gpaths[i].path = pathptr;
        gpaths[i].len = len;
        pathptr += len + 1;
    }
    sg_paths.path = gpaths;
    sg_paths.pathcount = count;
    sg_paths.maxlen = maxlen;

    free(userbuf);
    free(databuf);
    return;

error:
    sg_path_initabort();
}

#if defined _WIN32

static void
sg_path_init2(void)
{
    struct sg_path cvar[2];
    const char *cval;
    int i, r, wlen;
    sg_path_initcvar();
    for (i = 0; i < 2; i++) {
        cval = sg_path_cvar[i].value;
        if (*cval) {
            r = sg_wchar_from_utf8(&cvar[i].path, &wlen, cval, strlen(cval));
            if (r)
                goto error;
            cvar[i].len = wlen;
        } else {
            cvar[i].path = NULL;
            cvar[i].len = 0;
        }
    }
    sg_path_init3(cvar);
    for (i = 0; i < 2; i++)
        free(cvar[i].path);
    return;

error:
    sg_path_initabort();
}

#else

static void
sg_path_init2(void)
{
    struct sg_path cvar[2];
    char *cval;
    int i;
    for (i = 0; i < 2; i++) {
        cval = sg_paths.cvar[i].value;
        cvar[i].path = cval;
        cvar[i].len = strlen(cval);
    }
    sg_path_init3(cvar);
}

#endif

void
sg_path_init(void)
{
    sg_cvar_defstring(NULL, "userpath", &sg_paths.cvar[0],
                      NULL, SG_CVAR_INITONLY);
    sg_cvar_defstring(NULL, "datapath", &sg_paths.cvar[1],
                      NULL, SG_CVAR_INITONLY);
    sg_path_init2();
}
