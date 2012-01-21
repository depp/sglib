/* POSIX file / path code.  Used on Linux, BSD, Mac OS X.  */

/* This gives us 64-bit file offsets on 32-bit Linux */
#define _FILE_OFFSET_BITS 64

#include "error.h"
#include "file.h"
#include "file_impl.h"
#include "log.h"
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* Get the path to the running executable, or the main bundle on Mac
   OS X.  Store the result in a malloc'd buffer in buf/len and return
   1.  Return 0 if unimplemented, or -1 if an error occurs.  */
static int
sg_path_getexepath(char **buf, size_t *len);

void
sg_buffer_destroy(struct sg_buffer *fbuf)
{
    if (fbuf->data)
        free(fbuf->data);
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
        return r;
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
        return r;
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

int
sg_file_tryopen(struct sg_file **f, const char *path, int flags,
                struct sg_error **e)
{
    int fdes, ecode;
    struct sg_file_u *u;
    if (flags & SG_WRONLY)
        fdes = open(path, O_WRONLY, 0666);
    else
        fdes = open(path, O_RDONLY);
    if (fdes >= 0) {
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
    } else {
        ecode = errno;
        if (ecode == ENOENT)
            return 0;
        sg_error_errno(e, ecode);
        return -1;
    }
}

int
sg_path_getdir(pchar **abspath, size_t *abslen,
               const char *relpath, size_t rellen,
               int flags)
{
    struct sg_logger *logger;
    char *dpath = NULL, *apath = NULL, *rpath = NULL, *bpath = NULL, *real;
    size_t dlen, aalloc, rlen, blen;
    int r, ret, bslash, pslash, e;

    logger = sg_logger_get("path");

    /* FIXME: log errors / warnings */
    if (!rellen)
        return 0;
    if (memchr(relpath, '\0', rellen))
        return 0;

    pslash = relpath[rellen - 1] != '/';
    if (relpath[0] == '/') {
        rlen = rellen;
        rpath = malloc(rellen + pslash + 1);
        if (!rpath)
            goto nomem;
        memcpy(rpath, relpath, rellen);
        if (pslash)
            rpath[rlen++] = '/';
    } else {
        if (flags & SG_PATH_EXEDIR) {
            r = sg_path_getexepath(&bpath, &blen);
            if (!r)
                goto error;
            blen -= 1;
            while (blen > 0 && bpath[blen-1] != '/')
                blen--;
            if (!blen)
                goto error;
        } else {
            bpath = getcwd(0, 0);
            if (!bpath)
                goto error;
            blen = strlen(bpath);
            if (!blen)
                goto error;
        }
        bslash = bpath[blen - 1] != '/';
        rpath = malloc(blen + bslash + rellen + pslash + 1);
        if (!rpath)
            goto nomem;
        memcpy(rpath, bpath, blen);
        if (bslash)
            rpath[blen] = '/';
        memcpy(rpath + blen + bslash, relpath, rellen);
        if (pslash)
            rpath[blen + bslash + rellen] = '/';
        rlen = blen + bslash + rellen + pslash;
    }
    rpath[rlen] = '\0';

    aalloc = PATH_MAX > 0 ? PATH_MAX : 1024;
    apath = malloc(aalloc);
    if (!apath)
        goto nomem;
    real = realpath(rpath, apath);
    if (real) {
        dlen = strlen(real);
        pslash = real[dlen - 1] != '/';
        dpath = malloc(dlen + pslash + 1);
        if (!dpath)
            goto nomem;
        memcpy(dpath, real, dlen);
        if (pslash)
            dpath[dlen++] = '/';
        dpath[dlen] = '\0';
        r = access(dpath, F_OK | X_OK);
        if (r) {
            if (LOG_INFO >= logger->level) {
                e = errno;
                sg_logf(logger, LOG_INFO,
                        "path skipped: %s (%s)", dpath, strerror(e));
            }
            ret = 0;
        } else {
            if (LOG_INFO >= logger->level)
                sg_logf(logger, LOG_INFO, "path: %s", dpath);
            *abspath = dpath;
            *abslen = dlen;
            dpath = NULL;
            ret = 1;
        }
    } else if (flags & SG_PATH_NODISCARD) {
        if (LOG_INFO >= logger->level)
            sg_logf(logger, LOG_INFO, "path: %s", rpath);
        *abspath = rpath;
        *abslen = rlen;
        rpath = NULL;
        ret = 1;
    } else {
        if (LOG_INFO >= logger->level) {
            e = errno;
            sg_logf(logger, LOG_INFO, "path skipped: %s (%s)",
                    rpath, strerror(e));
        }
        ret = 0;
    }

    free(dpath);
    free(apath);
    free(rpath);
    free(bpath);
    return ret;

error:
    abort();

nomem:
    abort();
}

#if defined(__APPLE__)
#include <CoreFoundation/CFBundle.h>
#include <sys/param.h>

/* Mac OS X implementation */
static int
sg_path_getexepath(char **buf, size_t *len)
{
    size_t bsz = PATH_MAX > 0 ? PATH_MAX : 256;
    CFBundleRef bundle;
    CFURLRef url = NULL;
    Boolean r;
    int ret = 0;
    char *str = NULL;

    bundle = CFBundleGetMainBundle();
    if (!bundle)
        goto done;
    url = CFBundleCopyBundleURL(bundle);
    if (!url)
        goto done;
    str = malloc(bsz);
    if (!str)
        goto nomem;
    r = CFURLGetFileSystemRepresentation(
        url, true, (UInt8 *) str, bsz);
    if (!r)
        goto done;
    ret = 1;
    *len = strlen(str);
    *buf = str;
    str = NULL;

done:
    if (url)
        CFRelease(url);
    free(str);
    return ret;

nomem:
    abort();
}

#elif defined(__linux__)
#include <sys/param.h>

/* Linux implementation */
static int
sg_path_getexepath(char **buf, size_t *len)
{
    char *str;
    ssize_t sz;
    size_t rlen = PATH_MAX > 0 ? PATH_MAX : 1024;
    str = malloc(rlen);
    if (!str)
        return 0;
    sz = readlink("/proc/self/exe", str, rlen);
    if (sz > 0) {
        *buf = str;
        *len = sz;
        return 1;
    } else {
        free(str);
        return 0;
    }
}

#else

#warning "Can't find executable directory on this platform"

static int
sg_path_getexedir(char *buf, unsigned len)
{
    (void) buf;
    (void) len;
    return 0;
}

#endif
