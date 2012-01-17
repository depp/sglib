#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define SG_FILE_INITBUF 4096
#define SG_PATH_INITSZ 1

/* This gives us 64-bit file offsets on 32-bit Linux */
#define _FILE_OFFSET_BITS 64

#include "cvar.h"
#include "error.h"
#include "file.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#define SG_PATH_UNICODE 1
#define sg_file_i_open sg_file_w_open
#define sg_path_i_init sg_path_w_init
#define SG_PATH_SEP ';'
#else
#define sg_file_i_open sg_file_u_open
#define sg_path_i_init sg_path_u_init
#define SG_PATH_SEP ':'
#endif

#if defined(SG_PATH_UNICODE)
typedef wchar_t pchar;
#else
typedef char pchar;
#endif

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

/* Flags used by sg_path_add and sg_path_i_init.  */
enum {
    /* The path must be writable.  You should normally specify
       SG_PATH_NODISCARD as well, in which case the directory
       will be created if necessary.  */
    SG_PATH_WRITABLE = 1 << 0,
    /* Do not discard the path if it is not present.  */
    SG_PATH_NODISCARD = 1 << 1,
    /* The path is relative to the executable's directory.  */
    SG_PATH_EXEDIR = 1 << 2
};

/* Returns >0 for success, 0 for file not found, <0 for error.  */
static int
sg_file_i_open(struct sg_file **f, const pchar *path, int flags,
               struct sg_error **e);

/* Initialize the given sg_path and return >0, return <0 if an
   error occurred, or return 0 if the path should be discarded.  */
static int
sg_path_i_init(struct sg_path *p, const char *path, size_t len,
               int flags);

static struct sg_paths sg_paths;

struct sg_file *
sg_file_open(const char *path, int flags, struct sg_error **e)
{
    struct sg_file *f;
    struct sg_path *a;
    int nlen, r;
    unsigned i, pcount, pmaxlen;
    char nbuf[SG_MAX_PATH];
    pchar *pbuf = NULL, *p;

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
    if (!pcount)
        goto notfound;
    pbuf = malloc((pmaxlen + nlen + 1) * sizeof(pchar));
    if (!pbuf)
        goto nomem;
#if defined(_WIN32)
    for (i = 0; i <= (unsigned) nlen; ++i)
        pbuf[pmaxlen + i] = nbuf[i] == '/' ? '\\' : nbuf[i];
#else
    memcpy(pbuf + pmaxlen, nbuf, nlen + 1);
#endif
    for (i = 0; i < pcount; ++i) {
        p = pbuf + pmaxlen - a[i].len;
        memcpy(p, a[i].path, a[i].len * sizeof(pchar));
        r = sg_file_i_open(&f, p, flags, e);
        if (r > 0)
            goto done;
        if (r < 0) {
            f = NULL;
            goto done;
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
        len = (size_t) flen + 1;
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

/* Add a path to the given search paths.  */
static void
sg_path_add(struct sg_paths *p, const char *path, size_t len, int flags)
{
    struct sg_path npath, *na;
    unsigned nalloc, pos;
    int r;

    r = sg_path_i_init(&npath, path, len, flags);
    if (r <= 0)
        return;

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
        p->amaxlen = npath.len;
    p->acount += 1;
    if (flags & SG_PATH_WRITABLE) {
        pos = p->wcount;
        if (npath.len > p->wmaxlen)
            p->wmaxlen = npath.len;
        p->wcount += 1;
    }
    memmove(p->a + pos + 1, p->a + pos,
            sizeof(*p->a) * (p->acount - pos - 1));
    memcpy(p->a + pos, &npath, sizeof(npath));
    return;

nomem:
    abort();
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
        sep = strchr(p, SG_PATH_SEP);
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

#ifdef _WIN32
#include <Windows.h>

void
sg_buffer_destroy(struct sg_buffer *fbuf)
{
    if (fbuf->data)
        free(fbuf->data);
}

struct sg_file_w {
    struct sg_file h;
    HANDLE hdl;
};

static int
sg_file_w_read(struct sg_file *f, void *buf, size_t amt)
{
    struct sg_file_w *w = (struct sg_file_w *) f;
    DWORD ramt;
    if (ReadFile(w->hdl, buf, amt > INT_MAX ? INT_MAX : amt, &ramt, NULL)) {
        return ramt;
    } else {
        sg_error_win32(&w->h.err, GetLastError());
        return -1;
    }
}

static int
sg_file_w_write(struct sg_file *f, const void *buf, size_t amt)
{
    struct sg_file_w *w = (struct sg_file_w *) f;
    DWORD ramt;
    if (WriteFile(w->hdl, buf, amt > INT_MAX ? INT_MAX : amt, &ramt, NULL)) {
        return ramt;
    } else {
        sg_error_win32(&w->h.err, GetLastError());
        return -1;
    }
}

static int
sg_file_w_close(struct sg_file *f)
{
    struct sg_file_w *w = (struct sg_file_w *) f;
    if (w->hdl != INVALID_HANDLE_VALUE) {
        CloseHandle(w->hdl);
        w->hdl = INVALID_HANDLE_VALUE;
    }
    return 0;
}

static void
sg_file_w_free(struct sg_file *f)
{
    struct sg_file_w *w = (struct sg_file_w *) f;
    if (w->hdl != INVALID_HANDLE_VALUE)
        CloseHandle(w->hdl);
    free(w);
}

static int64_t
sg_file_w_length(struct sg_file *f)
{
    struct sg_file_w *w = (struct sg_file_w *) f;
    BY_HANDLE_FILE_INFORMATION ifo;
    if (GetFileInformationByHandle(w->hdl, &ifo)) {
        return ((int64_t) ifo.nFileSizeHigh << 32) | ifo.nFileSizeLow;
    } else {
        sg_error_win32(&w->h.err, GetLastError());
        return -1;
    }
}

static int64_t
sg_file_w_seek(struct sg_file *f, int64_t off, int whence)
{
    struct sg_file_w *w = (struct sg_file_w *) f;
    DWORD lo, hi, method, err;
    switch (whence) {
    case SEEK_SET: method = FILE_BEGIN; break;
    case SEEK_CUR: method = FILE_CURRENT; break;
    case SEEK_END: method = FILE_END; break;
    default: method = (DWORD) -1; break;
    }
    lo = off & 0xffffffffu;
    hi = off >> 32;
    lo = SetFilePointer(w->hdl, lo, &hi, method);
    if (lo == INVALID_SET_FILE_POINTER) {
        err = GetLastError();
        if (err) {
            sg_error_win32(&w->h.err, GetLastError());
            return -1;
        }
    }
    return ((int64_t) hi << 32) | lo;
}

static int
sg_file_w_open(struct sg_file **f, const wchar_t *path, int flags,
               struct sg_error **e)
{
    struct sg_file_w *w;
    HANDLE h;
    DWORD ecode;
    h = CreateFileW(path, FILE_READ_DATA, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h != INVALID_HANDLE_VALUE) {
        w = malloc(sizeof(*w));
        if (!w) {
            CloseHandle(h);
            sg_error_nomem(e);
            return -1;
        }
        w->h.refcount = 1;
        w->h.read = sg_file_w_read;
        w->h.write = sg_file_w_write;
        w->h.close = sg_file_w_close;
        w->h.free = sg_file_w_free;
        w->h.length = sg_file_w_length;
        w->h.seek = sg_file_w_seek;
        w->hdl = h;
        *f = &w->h;
        return 1;
    } else {
        ecode = GetLastError();
        if (ecode == ERROR_FILE_NOT_FOUND || ecode == ERROR_PATH_NOT_FOUND)
            return 0;
        sg_error_win32(e, ecode);
        return -1;
    }
}

static int
sg_path_w_init(struct sg_path *p, const char *path, size_t len,
               int flags)
{
    /* Relative, absolute, executable, and working directory path.  */
    wchar_t *rpath = NULL, *apath = NULL, *epath = NULL, *wpath = NULL;
    size_t rlen, alen, elen, wlen;
    int r, ret;
    DWORD dr;
    BOOL br;

    /* FIXME: log errors / warnings */
    if (!len)
        return 0;
    if (memchr(path, '\0', len))
        return 0;

    /* Convert relative path to Unicode.  */
    r = MultiByteToWideChar(CP_UTF8, 0, path, len, NULL, 0);
    if (!r)
        goto error;
    rlen = r;
    rpath = malloc(sizeof(wchar_t) * (rlen + 2));
    if (!rpath)
        goto nomem;
    r = MultiByteToWideChar(CP_UTF8, 0, path, len, rpath, rlen);
    if (!r)
        goto error;

    /* Append backslash (if not present) and NUL.  */
    if (rpath[rlen-1] != L'\\')
        rpath[rlen++] = L'\\';
    rpath[rlen] = L'\0';

    if (flags & SG_PATH_EXEDIR) {
        /* If we want something relative to the executable directory,
           we change directory there and back afterwards.  */

        /* Get executable directory path */
        elen = MAX_PATH;
        while (1) {
            epath = malloc(sizeof(wchar_t) * elen);
            if (!epath)
                goto nomem;
            dr = GetModuleFileNameW(NULL, epath, elen);
            if (!dr)
                goto error;
            if (dr < elen) {
                elen = dr;
                break;
            }
            elen *= 2;
            free(epath);
        }
        elen = dr;
        while (elen > 0 && epath[elen - 1] != L'\\')
            elen--;
        epath[elen] = L'\0';

        /* Save current path */
        dr = GetCurrentDirectoryW(0, NULL);
        if (!dr)
            goto error;
        wpath = malloc(sizeof(wchar_t) * dr);
        if (!wpath)
            goto nomem;
        dr = GetCurrentDirectoryW(dr, wpath);
        if (!dr)
            goto error;
        wlen = dr;

        /* Change path */
        br = SetCurrentDirectoryW(epath);
        if (!br)
            goto error;
    }

    dr = GetFullPathNameW(rpath, 0, NULL, NULL);
    if (!dr)
        goto error;
    apath = malloc(sizeof(wchar_t) * dr);
    dr = GetFullPathNameW(rpath, dr, apath, NULL);
    if (!dr)
        goto error;
    alen = dr;

    if (!(flags & SG_PATH_NODISCARD)) {
        dr = GetFileAttributesW(apath);
        if (dr == INVALID_FILE_ATTRIBUTES) {
            dr = GetLastError();
            if (dr == ERROR_FILE_NOT_FOUND || dr == ERROR_PATH_NOT_FOUND)
                ret = 0;
            else
                goto errorcode;
        } else if (!(dr & FILE_ATTRIBUTE_DIRECTORY)) {
            ret = 0;
        } else {
            ret = 1;
        }
    } else {
        ret = 1;
    }

    if (ret > 0) {
        p->path = apath;
        p->len = alen;
        apath = NULL;
    }

done:
    if (wpath)
        SetCurrentDirectoryW(wpath);
    free(rpath);
    free(apath);
    free(epath);
    free(wpath);
    return ret;

error:
    dr = GetLastError();
    goto errorcode;

errorcode:
    abort();
    goto done;
    
nomem:
    abort();
    goto done;
}

#else

#include "cvar.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
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

static int
sg_file_u_open(struct sg_file **f, const char *path, int flags,
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

/* Return 1 for success, 0 for failure.  */
static int
sg_path_getexepath(char **buf, size_t *len);

static int
sg_path_u_init(struct sg_path *p, const char *path, size_t len,
               int flags)
{
    char *dpath = NULL, *apath = NULL, *rpath = NULL, *bpath = NULL, *real;
    size_t dlen, aalloc, rlen, blen;
    int r, ret, bslash, pslash;

    /* FIXME: log errors / warnings */
    if (!len)
        return 0;
    if (memchr(path, '\0', len))
        return 0;

    pslash = path[len - 1] != '/';
    if (path[0] == '/') {
        rlen = len;
        rpath = malloc(len + pslash + 1);
        if (!rpath)
            goto nomem;
        memcpy(rpath, path, len);
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
        rpath = malloc(blen + bslash + len + pslash + 1);
        if (!rpath)
            goto nomem;
        memcpy(rpath, bpath, blen);
        if (bslash)
            rpath[blen] = '/';
        memcpy(rpath + blen + bslash, path, len);
        if (pslash)
            rpath[blen + bslash + len] = '/';
        rlen = blen + bslash + len + pslash;
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
            ret = 0;
        } else {
            p->path = dpath;
            p->len = dlen;
            dpath = NULL;
            ret = 1;
        }
    } else if (flags & SG_PATH_NODISCARD) {
        p->path = rpath;
        p->len = rlen;
        rpath = NULL;
        ret = 1;
    } else {
        ret = 0;
    }
    if (ret)
        fprintf(stderr, "path: %s\n", p->path);

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

#endif
