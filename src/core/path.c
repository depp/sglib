/* Copyright 2012-2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "file_impl.h"
#include "sg/cvar.h"
#include "sg/defs.h"
#include "sg/entry.h"
#include "sg/error.h"
#include "sg/file.h"
#include "sg/log.h"
#include "private.h"

#include <stdlib.h>

#if defined _WIN32
# include <Windows.h>
#endif

#define SG_PATH_BUFSZ 1024

struct sg_paths sg_paths;

SG_ATTR_NORETURN
static void
sg_path_initabort(void)
{
    sg_sys_abort("Failed to initialize paths.");
}

pchar *
sg_file_createpath(const char *path, size_t pathlen,
                   struct sg_error **err)
{
    int r;
    pchar *result;
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

    r = sg_path_mkpardir(result, err);
    if (r != SG_FILE_OK) {
        free(result);
        if (r != SG_FILE_ERROR)
            sg_error_notfound(err, nbuf);
        return NULL;
    }

    return result;
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
    int i, j, r, wlen;
    wchar_t *wpath;
    for (i = 0; i < 2; i++) {
        cval = sg_paths.cvar[i].value;
        if (*cval) {
            r = sg_wchar_from_utf8(&wpath, &wlen, cval, strlen(cval));
            if (r)
                goto error;
            for (j = 0; j < wlen; j++)
                if (wpath[j] == '/')
                    wpath[j] = '\\';
            cvar[i].path = wpath;
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
    sg_cvar_defstring(
        NULL, "userpath", "The path where user data is stored.",
        &sg_paths.cvar[0], NULL, SG_CVAR_INITONLY);
    sg_cvar_defstring(
        NULL, "datapath", "The paths where data files are found.",
        &sg_paths.cvar[1], NULL, SG_CVAR_INITONLY);
    sg_path_init2();
}
