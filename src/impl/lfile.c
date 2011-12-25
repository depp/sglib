#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define SG_FILE_INITBUF 4096
#define SG_PATH_INITSZ 1
#define _FILE_OFFSET_BITS 64

#include "error.h"
#include "lfile.h"
#include "strbuf.h"
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

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

struct sg_path {
    char *path;
    size_t len;
};

struct sg_paths {
    struct sg_path *a;
    unsigned wcount, acount;
    unsigned wmaxlen, amaxlen;
    unsigned alloc;
};

static struct sg_paths sg_paths;

struct sg_file *
sg_file_open(const char *path, int flags, struct sg_error **e)
{
    struct sg_file *f;
    struct sg_file_u *u;
    struct sg_path *a;
    int nlen, fdes, fdflags, ecode;
    unsigned i, pcount, pmaxlen;
    char nbuf[SG_MAX_PATH];
    char *pbuf = NULL, *p;

    flags = flags;
    if (flags & SG_WRONLY)
        flags |= SG_WRITABLE;

    nlen = sg_path_norm(nbuf, path, e);
    if (nlen < 0)
        return NULL;

    a = sg_paths.a;
    if (flags & SG_LOCAL) {
        pcount = sg_paths.wcount;
        pmaxlen = sg_paths.wmaxlen;
    } else {
        pcount = sg_paths.acount;
        pmaxlen = sg_paths.amaxlen;
    }
    if (flags & SG_WRONLY)
        fdflags = O_WRONLY;
    else
        fdflags = O_RDONLY;
    if (!pcount)
        goto notfound;
    pbuf = malloc(pmaxlen + nlen + 1);
    if (!pbuf)
        goto nomem;
    memcpy(pbuf + pmaxlen, nbuf, nlen + 1);
    for (i = 0; i < pcount; ++i) {
        p = pbuf + pmaxlen - a[i].len;
        memcpy(p, a[i].path, a[i].len);
        fdes = open(p, fdflags);
        if (fdes >= 0) {
            u = malloc(sizeof(*u));
            if (!u)
                goto nomem;
            u->h.refcount = 1;
            u->h.read = sg_file_u_read;
            u->h.write = sg_file_u_write;
            u->h.close = sg_file_u_close;
            u->h.free = sg_file_u_free;
            u->h.length = sg_file_u_length;
            u->h.seek = sg_file_u_seek;
            u->fdes = fdes;
            f = &u->h;
            goto done;
        } else {
            ecode = errno;
            if (ecode != ENOENT) {
                sg_error_errno(e, ecode);
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

static void
sg_path_add(struct sg_paths *p, const char *path, unsigned len,
            int writable)
{
    unsigned nalloc, pos;
    struct sg_path *na;
    char *npath;

    if (!len) {
        fprintf(stderr, "warning: sg_path_add: empty path\n");
        return;
    }
    while (len && path[len - 1] == '/')
        len--;
    npath = malloc(len + 1);
    if (!npath)
        abort();
    memcpy(npath, path, len);
    npath[len] = '/';
    len++;
    if (p->acount >= p->alloc) {
        nalloc = p->alloc ? p->alloc * 2 : SG_PATH_INITSZ;
        na = realloc(p->a, sizeof(*na) * nalloc);
        if (!na)
            abort();
        p->a = na;
        p->alloc = nalloc;
    }
    pos = p->acount;
    if (len > p->amaxlen)
        p->amaxlen = len;
    p->acount += 1;
    if (writable) {
        pos = p->wcount;
        if (len > p->wmaxlen)
            p->wmaxlen = len;
        p->wcount += 1;
    }
    memmove(p->a + pos + 1, p->a + pos,
            sizeof(*p->a) * (p->acount - pos - 1));
    p->a[pos].path = npath;
    p->a[pos].len = len;
}

#if defined(__APPLE__)
#include <CoreFoundation/CFBundle.h>
#include <sys/param.h>

/* Mac OS X implementation */
static int
sg_path_getexepath(struct sg_strbuf *b)
{
    CFBundleRef main;
    CFURLRef url = NULL;
    Boolean r;
    unsigned l;
    int ret = -1;
    size_t rlen = PATH_MAX > 256 ? PATH_MAX : 256;

    sg_strbuf_clear(b);
    sg_strbuf_reserve(b, rlen - 1);

    main = CFBundleGetMainBundle();
    if (!main)
        goto done;
    url = CFBundleCopyBundleURL(main);
    if (!url)
        goto done;
    r = CFURLGetFileSystemRepresentation(
        url, true, (UInt8 *) b->s, rlen);
    if (!r)
        goto done;
    sg_strbuf_forcelen(b, strlen(b->s));
    ret = 0;

done:
    if (url)
        CFRelease(url);
    return ret;
}

#elif defined(__linux__)
#include <sys/param.h>

/* Linux implementation */
static int
sg_path_getexepath(struct sg_strbuf *b)
{
    ssize_t sz;
    size_t rlen = PATH_MAX > 256 ? PATH_MAX : 256;
    sg_strbuf_clear(b);
    sg_strbuf_reserve(b, rlen - 1);
    sz = readlink("/proc/self/exe", b->s, rlen - 1);
    if (sz < 0)
        return -1;
    sg_strbuf_forcelen(b, sz);
    return 0;
}

#else

#warning "Can't find executable directory on this platform"

static int
sg_path_getexedir(char *buf, unsigned len)
{
    (void) buf;
    (void) len;
    return -1;
}

#endif

void
sg_path_init(void)
{
    struct sg_strbuf buf;
    int r;
    sg_strbuf_init(&buf, 0);
    r = sg_path_getexepath(&buf);
    if (!r) {
        sg_strbuf_getdir(&buf);
        sg_path_add(&sg_paths, buf.s, sg_strbuf_len(&buf), 1);
    }
    sg_strbuf_destroy(&buf);
}

int
sg_file_readall(struct sg_file *f, struct sg_buffer *fbuf, size_t maxsize)
{
    unsigned char *buf = NULL, *nbuf;
    size_t len, nlen, pos;
    int64_t flen;
    int amt;
    int (*read)(struct sg_file *f, void *buf, size_t amt);
    read = f->read;

    if (f->length) {
        flen = f->length(f);
        if (flen < 0)
            return -1;
        if (flen > maxsize)
            goto toobig;
        len = flen + 1;
    } else {
        len = maxsize > SG_FILE_INITBUF ? SG_FILE_INITBUF : (maxsize + 1);
    }

    buf = malloc(len);
    if (!buf)
        goto nomem;
    pos = 0;
    while (1) {
        amt = read(f, buf + pos, len - pos);
        if (amt > 0) {
            pos += amt;
            if (pos > maxsize)
                goto toobig;
            if (pos >= len) {
                nlen = len * 2;
                if (!nlen)
                    goto nomem;
                nbuf = realloc(buf, nlen);
                if (!nbuf)
                    goto nomem;
                buf = nbuf;
                len = nlen;
            }
        } else if (amt == 0) {
            break;
        } else {
            goto err;
        }
    }
    buf[pos] = '\0';
    if (pos + 1 < len)
        buf = realloc(buf, pos + 1);
    fbuf->data = buf;
    fbuf->length = pos;
    return 0;

nomem:
    sg_error_nomem(&f->err);
    goto err;

err:
    free(buf);
    return -1;

toobig:
    free(buf);
    sg_error_sets(&f->err, &SG_ERROR_DATA, 0, "file too large");
    return 1;
}

int
sg_file_get(const char *path, int flags,
            struct sg_buffer *fbuf, size_t maxsize, struct sg_error **e)
{
    struct sg_file *f;
    int r;
    f = sg_file_open(path, flags, e);
    if (!f)
        return -1;
    r = sg_file_readall(f, fbuf, maxsize);
    if (r)
        sg_error_move(e, &f->err);
    f->free(f);
    return r;
}
