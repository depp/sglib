/* File / path convenience functions for reading files */

#define SG_FILE_INITBUF 4096

#include "error.h"
#include "file.h"
#include <stdlib.h>

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
        if ((uint64_t) flen > maxsize)
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
sg_file_get(const char *path, size_t pathlen, int flags,
            const char *extensions,
            struct sg_buffer *fbuf, size_t maxsize, struct sg_error **e)
{
    struct sg_file *f;
    int r;
    f = sg_file_open(path, pathlen, flags, extensions, e);
    if (!f)
        return -1;
    r = sg_file_readall(f, fbuf, maxsize);
    if (r)
        sg_error_move(e, &f->err);
    f->free(f);
    return r;
}
