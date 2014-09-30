/* Copyright 2012 Dietrich Epp.
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

void
sg_buffer_free_(struct sg_buffer *fbuf)
{
    if (fbuf->data)
        free(fbuf->data);
    free(fbuf);
}

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
sg_file_u_close(struct sg_file *f)
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
sg_file_u_free(struct sg_file *f)
{
    struct sg_file_u *u = (struct sg_file_u *) f;
    if (u->fdes >= 0)
        close(u->fdes);
    free(u);
}

static int64_t
sg_file_u_length(struct sg_file *f)
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
    return st.st_size;
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

/* Make the parent directory for a file, recursively.  */
static int
sg_file_mkpardir(const char *path, struct sg_error **err)
{
    char *buf, *p;
    size_t len;
    int r, ecode, ret = 0;

    p = strrchr(path, '/');
    if (!p || p == path)
        return 0;
    len = p - path;
    buf = malloc(len + 1);
    if (!buf) {
        sg_error_nomem(err);
        return -1;
    }
    memcpy(buf, path, len);
    buf[len] = '\0';
    while (1) {
        r = mkdir(buf, 0777);
        if (!r) {
            /* printf("mkdir %s\n", buf); */
            break;
        }
        ecode = errno;
        if (ecode != ENOENT) {
            sg_error_errno(err, ecode);
            ret = -1;
            goto done;
        }
        p = strrchr(buf, '/');
        if (!p || p == buf)
            goto done;
        *p = '\0';
    }
    while (1) {
        *p = '/';
        p += strlen(p);
        if (p == buf + len)
            goto done;
        r = mkdir(buf, 0777);
        if (r) {
            ecode = errno;
            sg_error_errno(err, ecode);
            ret = -1;
            goto done;
        }
        /* printf("mkdir %s\n", buf); */
    }
done:
    free(buf);
    return ret;
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
            if (sg_file_mkpardir(path, e))
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
    u->h.refcount = 1;
    u->h.read = sg_file_u_read;
    u->h.write = sg_file_u_write;
    u->h.close = sg_file_u_close;
    u->h.free = sg_file_u_free;
    u->h.length = sg_file_u_length;
    u->h.seek = sg_file_u_seek;
    u->fdes = fdes;
    *f = &u->h;
    return 1;
}

int
sg_path_checkdir(const pchar *path)
{
    struct sg_logger *logger;
    int r, e;
    logger = sg_logger_get("path");
    r = access(path, R_OK | X_OK);
    if (r) {
        e = errno;
        if (SG_LOG_INFO >= logger->level) {
            sg_logf(logger, SG_LOG_INFO,
                    "path skipped: %s (%s)", path, strerror(e));
        }
        return 0;
    } else {
        sg_logf(logger, SG_LOG_INFO, "path: %s", path);
        return 1;
    }
}

#if defined(__APPLE__)
#include <CoreFoundation/CFBundle.h>
#include <sys/param.h>

/* Mac OS X implementation */
int
sg_path_getexepath(char *buf, size_t len)
{
    CFBundleRef bundle;
    CFURLRef url = NULL;
    Boolean r;
    int ret = 0;

    bundle = CFBundleGetMainBundle();
    if (!bundle)
        goto done;
    url = CFBundleCopyBundleURL(bundle);
    if (!url)
        goto done;
    r = CFURLGetFileSystemRepresentation(
        url, true, (UInt8 *) buf, len);
    if (!r)
        goto done;
    ret = 1;

done:
    if (url)
        CFRelease(url);
    return ret;
}

#elif defined(__linux__)
#include <sys/param.h>

/* Linux implementation */
int
sg_path_getexepath(char *buf, size_t len)
{
    ssize_t r;
    r = readlink("/proc/self/exe", buf, len - 1);
    if (r > 0 && (size_t) r < len - 1) {
        buf[r] = '\0';
        return 1;
    } else {
        return 0;
    }
}

#else

#warning "Can't find executable directory on this platform"

int
sg_path_getexepath(char *buf, size_t len)
{
    (void) buf;
    (void) len;
    return -1;
}

#endif
