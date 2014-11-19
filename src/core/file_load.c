/* Copyright 2012-2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "file_impl.h"
#include "sg/error.h"
#include "sg/file.h"
#include "sg/log.h"
#include <stdlib.h>
#include <string.h>

static void
sg_filedata_free_(struct sg_filedata *data)
{
    if (data->data)
        free(data->data);
    free(data);
}

void
sg_filedata_incref(struct sg_filedata *data)
{
    sg_atomic_inc(&data->refcount_);
}

void
sg_filedata_decref(struct sg_filedata *data)
{
    int c = sg_atomic_fetch_add(&data->refcount_, -1);
    if (c == 1)
        sg_filedata_free_(data);
}

struct sg_filedata *
sg_reader_load(
    struct sg_reader *fp,
    size_t size,
    const char *path,
    size_t pathlen,
    struct sg_error **err)
{
    struct sg_filedata *dp;
    unsigned char *buf = NULL;
    char *pp;
    size_t pos;
    int r;

    buf = malloc(size + 1);
    if (!buf)
        goto nomem;
    pos = 0;
    while (pos < size) {
        r = sg_reader_read(fp, buf + pos, size - pos, err);
        if (r < 0)
            goto nomem;
        if (r == 0)
            break;
        pos += r;
    }
    buf[pos] = '\0';
    if (pos < size)
        buf = realloc(buf, pos + 1);
    dp = malloc(sizeof(*dp) + pathlen + 1);
    if (!dp)
        goto nomem;
    pp = (char *) (dp + 1);
    sg_atomic_set(&dp->refcount_, 1);
    dp->data = buf;
    dp->length = pos;
    dp->path = pp;
    dp->pathlen = pathlen;
    memcpy(pp, path, pathlen);
    pp[pathlen] = '\0';
    return dp;

nomem:
    free(buf);
    sg_error_nomem(err);
    return NULL;
}

struct sg_file_ext {
    const char *p;
    unsigned len;
};

int
sg_file_load(
    struct sg_filedata **data,
    const char *path,
    size_t pathlen,
    int flags,
    const char *extensions,
    size_t maxsize,
    struct sg_fileid *fileid,
    struct sg_error **err)
{
    struct sg_reader fp;
    char nbuf[SG_MAX_PATH];
    pchar *pbuf = NULL;
    struct sg_path *search;
    unsigned sflags, searchcount, maxslen;
    int nlen;

    sflags = flags & (SG_USERONLY | SG_DATAONLY);
    search = sg_paths.path;
    searchcount = sg_paths.pathcount;
    maxslen = sg_paths.maxlen;
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
        sg_error_invalid(err, __FUNCTION__, "flags");
        goto error;
    }
    if (searchcount == 1)
        maxslen = search[0].len;

    nlen = sg_path_norm(nbuf, path, pathlen, err);
    if (nlen < 0)
        return SG_FILE_ERROR;

    if (extensions) {
        struct sg_file_ext exts[SG_PATH_MAXEXTS];
        const char *extp = extensions, *ext, *extsep;
        pchar *pptr;
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
            if ((size_t) nlen > extlen && nbuf[nlen - extlen - 1] == '.' &&
                !memcmp(nbuf + nlen - extlen, ext, extlen)) {
                sg_logf(SG_LOG_WARN,
                        "Removing extension '.%.*s' from path: %s",
                        (int) extlen, ext, nbuf);
                nlen -= extlen + 1;
            }
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
            return SG_FILE_ERROR;
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
                r = sg_reader_open(&fp, pptr, err);
                if (r == SG_FILE_OK) {
                    nbuf[nlen++] = '.';
                    memcpy(nbuf + nlen, ext, extlen);
                    nlen += extlen;
                    nbuf[nlen] = '\0';
                    goto success;
                } else if (r == SG_FILE_ERROR) {
                    goto error;
                }
            }
        }
    } else {
        pchar *pptr;
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
            r = sg_reader_open(&fp, pptr, err);
            if (r == SG_FILE_OK) {
                goto success;
            } else if (r == SG_FILE_ERROR) {
                goto error;
            }
        }
    }

    goto notfound;

success:
    free(pbuf);
    pbuf = NULL;
    {
        int64_t flen;
        struct sg_fileid fi;
        struct sg_filedata *dp;
        int r, i;

        r = sg_reader_getinfo(&fp, &flen, &fi, err);
        if (r) {
            sg_reader_close(&fp);
            return SG_FILE_ERROR;
        }
        if ((uint64_t) flen > maxsize) {
            sg_error_sets(err, &SG_ERROR_DATA, 0, "file is too large");
            sg_reader_close(&fp);
            return SG_FILE_ERROR;
        }
        if (flags & SG_IFCHANGED) {
            for (i = 0; i < 3; i++)
                if (fi.f_[i] != fileid->f_[i])
                    break;
            if (i == 3) {
                sg_reader_close(&fp);
                return SG_FILE_NOTCHANGED;
            }
        }
        dp = sg_reader_load(&fp, flen, nbuf, nlen, err);
        sg_reader_close(&fp);
        if (!dp)
            return SG_FILE_ERROR;
        *data = dp;
        if (fileid)
            *fileid = fi;
    }
    return SG_FILE_OK;

notfound:
    sg_error_notfound(err, nbuf);
    goto error;

nomem:
    sg_error_nomem(err);
    goto error;

error:
    free(pbuf);
    return SG_FILE_ERROR;
}
