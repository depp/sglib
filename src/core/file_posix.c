/* Copyright 2012-2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
/* POSIX file / path code.  Used on Linux, BSD, Mac OS X.  */

/* This gives us 64-bit file offsets on 32-bit Linux */
#define _FILE_OFFSET_BITS 64

#include "file_impl.h"
#include "sg/error.h"
#include "sg/file.h"
#include "sg/log.h"
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

struct sg_file_u {
    struct sg_file h;
    int fdes;
};

static int
sg_file_u_read(struct sg_file *f, void *buf, size_t amt)
{
    struct sg_file_u *u = (struct sg_file_u *) f;
    ssize_t r;
    if (amt > INT_MAX)
        amt = INT_MAX;
    r = read(u->fdes, buf, amt);
    if (r >= 0)
        return (int) r;
    sg_error_errno(&u->h.err, errno);
    return -1;
}

static int
sg_file_u_write(struct sg_file *f, const void *buf, size_t amt)
{
    struct sg_file_u *u = (struct sg_file_u *) f;
    ssize_t r;
    if (amt > INT_MAX)
        amt = INT_MAX;
    r = write(u->fdes, buf, amt);
    if (r >= 0)
        return (int) r;
    sg_error_errno(&u->h.err, errno);
    return -1;
}

static int
sg_file_u_commit(struct sg_file *f)
{
    struct sg_file_u *u = (struct sg_file_u *) f;
    int r;
    if (u->fdes < 0)
        return 0;
    r = close(u->fdes);
    u->fdes = -1;
    if (r)
        sg_error_errno(&u->h.err, errno);
    return r;
}

static void
sg_file_u_close(struct sg_file *f)
{
    struct sg_file_u *u = (struct sg_file_u *) f;
    if (u->fdes >= 0)
        close(u->fdes);
    sg_error_clear(&u->h.err);
    free(u);
}

static int64_t
sg_file_u_seek(struct sg_file *f, int64_t off, int whence)
{
    struct sg_file_u *u = (struct sg_file_u *) f;
    off_t r;
    r = lseek(u->fdes, off, whence);
    if (r >= 0)
        return r;
    sg_error_errno(&u->h.err, errno);
    return -1;
}

static int
sg_file_u_getinfo(struct sg_file *f, struct sg_fileinfo *info)
{
    struct sg_file_u *u = (struct sg_file_u *) f;
    struct stat st;
    int r;
    r = fstat(u->fdes, &st);
    if (r) {
        sg_error_errno(&u->h.err, errno);
        return -1;
    }
    if (!S_ISREG(st.st_mode)) {
        sg_error_errno(&u->h.err, ESPIPE);
        return -1;
    }
    info->size = st.st_size;
    info->mtime = ((uint64_t) st.st_mtim.tv_sec << 32) |
        ((uint32_t) st.st_mtim.tv_nsec);
    info->volume = st.st_dev;
    info->ident = st.st_ino;
    return 0;
}

int
sg_file_tryopen(struct sg_file **f, const char *path, int flags,
                struct sg_error **e)
{
    int fdes, ecode;
    struct sg_file_u *u;
    if (flags & SG_WRONLY) {
        fdes = open(path, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0666);
        if (fdes < 0) {
            ecode = errno;
            if (ecode != ENOENT) {
                sg_error_errno(e, ecode);
                return -1;
            }
            if (sg_path_mkpardir(path, e))
                return -1;
            fdes = open(path, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0666);
        }
    } else {
        fdes = open(path, O_RDONLY | O_CLOEXEC);
    }
    if (fdes < 0) {
        ecode = errno;
        if (ecode != ENOENT) {
            sg_error_errno(e, ecode);
            return -1;
        }
        return 0;
    }
    u = malloc(sizeof(*u));
    if (!u) {
        close(fdes);
        sg_error_nomem(e);
        return -1;
    }
    u->h.read = sg_file_u_read;
    u->h.write = sg_file_u_write;
    u->h.commit = sg_file_u_commit;
    u->h.close = sg_file_u_close;
    u->h.seek = sg_file_u_seek;
    u->h.getinfo = sg_file_u_getinfo;
    u->h.err = NULL;
    u->fdes = fdes;
    *f = &u->h;
    return 1;
}
