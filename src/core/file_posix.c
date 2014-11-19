/* Copyright 2012-2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
/* POSIX file / path code.  Used on Linux, BSD, Mac OS X.  */

/* This gives us 64-bit file offsets on 32-bit Linux */
#define _FILE_OFFSET_BITS 64

#include "file_impl.h"
#include "sg/error.h"
#include "sg/file.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

int
sg_reader_open(
    struct sg_reader *fp,
    const pchar *path,
    struct sg_error **err)
{
    int fdes, ecode;
    fdes = open(path, O_RDONLY | O_CLOEXEC);
    if (fdes < 0) {
        ecode = errno;
        if (ecode == ENOENT)
            return SG_FILE_NOTFOUND;
        sg_error_errno(err, ecode);
        return SG_FILE_ERROR;
    }
    fp->fdes = fdes;
    return SG_FILE_OK;
}

int
sg_reader_getinfo(
    struct sg_reader *fp,
    int64_t *length,
    struct sg_fileid *fileid,
    struct sg_error **err)
{
    struct stat st;
    int r;
    r = fstat(fp->fdes, &st);
    if (r) {
        sg_error_errno(err, errno);
        return -1;
    }
    if (!S_ISREG(st.st_mode)) {
        sg_error_errno(err, ESPIPE);
        return -1;
    }
    *length = st.st_size;
#if defined __linux__
    fileid->f_[0] = ((uint64_t) st.st_mtim.tv_sec << 32) |
        ((uint32_t) st.st_mtim.tv_nsec);
#elif defined __APPLE__
    fileid->f_[0] = ((uint64_t) st.st_mtimespec.tv_sec << 32) |
        ((uint32_t) st.st_mtimespec.tv_nsec);
#endif
    fileid->f_[1] = st.st_ino;
    fileid->f_[2] = st.st_dev;
    return 0;
}

int
sg_reader_read(
    struct sg_reader *fp,
    void *buf,
    size_t bufsz,
    struct sg_error **err)
{
    size_t amt;
    ssize_t r;
    amt = bufsz > INT_MAX ? INT_MAX : bufsz;
    r = read(fp->fdes, buf, amt);
    if (r < 0)
        sg_error_errno(err, errno);
    return r;
}

void
sg_reader_close(
    struct sg_reader *fp)
{
    close(fp->fdes);
}

/*
  See Theo Ts'o's blog post for the reasoning behind how we write files.

  http://thunk.org/tytso/blog/2009/03/15/dont-fear-the-fsync/
*/
struct sg_writer {
    int fdes;
    char *destpath;
    char *temppath;
};

struct sg_writer *
sg_writer_open(
    const char *path,
    size_t pathlen,
    struct sg_error **err)
{
    struct sg_writer *fp;
    const char *basepath;
    size_t baselen;
    char buf[SG_MAX_PATH], *destpath, *temppath;
    int nlen, fdes, ecode, flags, mode;

    nlen = sg_path_norm(buf, path, pathlen, err);
    if (nlen < 0)
        return NULL;

    basepath = sg_paths.path[0].path;
    baselen = sg_paths.path[0].len;
    fp = malloc(sizeof(*fp) + (baselen + nlen) * 2 + 6);
    if (!fp)
        goto error_nomem;
    fp->destpath = destpath = (char *) (fp + 1);
    fp->temppath = temppath = destpath + baselen + nlen + 1;
    memcpy(destpath, basepath, baselen);
    memcpy(destpath + baselen, buf, nlen + 1);
    memcpy(temppath, basepath, baselen);
    memcpy(temppath + baselen, buf, nlen);
    memcpy(temppath + baselen + nlen, ".tmp", 5);

    flags = O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC;
    mode = 0666;
    fdes = open(temppath, flags, mode);
    if (fdes < 0) {
        ecode = errno;
        if (ecode != ENOENT)
            goto error_errno;
        if (sg_path_mkpardir(temppath, err))
            goto error;
        fdes = open(temppath, flags, mode);
        if (fdes < 0) {
            ecode = errno;
            goto error_errno;
        }
    }
    fp->fdes = fdes;
    return fp;

error_nomem:
    sg_error_nomem(err);
    return NULL;

error_errno:
    sg_error_errno(err, ecode);
    goto error;

error:
    free(fp);
    return NULL;
}

int64_t
sg_writer_seek(
    struct sg_writer *fp,
    int64_t offset,
    int whence,
    struct sg_error **err)
{
    int64_t r;
    if (fp->fdes < 0) {
        sg_error_invalid(err, __FUNCTION__, "fp");
        return -1;
    }
    r = lseek(fp->fdes, offset, whence);
    if (r < 0)
        sg_error_errno(err, errno);
    return r;
}

int
sg_writer_write(
    struct sg_writer *fp,
    const void *buf,
    size_t amt,
    struct sg_error **err)
{
    size_t namt;
    ssize_t r;
    if (fp->fdes < 0) {
        sg_error_invalid(err, __FUNCTION__, "fp");
        return -1;
    }
    namt = amt > INT_MAX ? INT_MAX : amt;
    r = write(fp->fdes, buf, namt);
    if (r < 0)
        sg_error_errno(err, errno);
    return r;
}

int
sg_writer_commit(
    struct sg_writer *fp,
    struct sg_error **err)
{
    int fdes, r, ecode;
    fdes = fp->fdes;
    if (fdes < 0) {
        sg_error_invalid(err, __FUNCTION__, "fp");
        return -1;
    }
    fp->fdes = -1;
    r = fsync(fdes);
    if (r) {
        close(fdes);
        goto error_errno;
    }
    r = close(fdes);
    if (r)
        goto error_errno;
#if defined __APPLE__
    /* This preserves metadata, other than modification time.  */
    r = exchangedata(fp->temppath, fp->destpath, 0);
    if (r) {
        ecode = errno;
        switch (ecode) {
        case ENOTSUP:
        case ENOENT:
            break;
        default:
            goto error_ecode;
        }
    }
#endif
    r = rename(fp->temppath, fp->destpath);
    if (r)
        goto error_errno;
    return 0;

error_errno:
    ecode = errno;
    goto error_ecode;

error_ecode:
    sg_error_errno(err, ecode);
    return -1;
}

void
sg_writer_close(
    struct sg_writer *fp)
{
    if (fp->fdes >= 0) {
        close(fp->fdes);
        unlink(fp->temppath);
    }
    free(fp);
}
