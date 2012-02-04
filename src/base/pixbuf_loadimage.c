/* Maximum size of an image file in bytes.  */
#define IMAGE_MAXSIZE (16 * 1024 * 1024)

#include "error.h"
#include "file.h"
#include "pixbuf.h"
#include <string.h>

enum {
    IMG_PNG,
    IMG_JPEG
};

int
sg_pixbuf_loadimage(struct sg_pixbuf *pbuf, const char *path,
                    struct sg_error **err)
{
    struct sg_buffer fbuf;
    char npath[SG_MAX_PATH + 8];
    int r, c, attempt = -1;
    size_t l, maxsize;
    unsigned e, i;
    struct sg_error *ferr = NULL;

    maxsize = IMAGE_MAXSIZE;
    l = strlen(path);
    if (l >= SG_MAX_PATH)
        return -1;
    for (e = l; e && path[e-1] != '/' && path[e-1] != '.'; --e) { }
    if (!e || path[e-1] == '/') {
        /* The path has no extension.  */
        e = l;
    } else {
        /* The path has an extension.  */
        for (i = e; i < l; ++i) {
            c = path[i];
            if (c >= 'A' && c <= 'Z')
                c += 'a' - 'A';
            npath[i] = c;
        }
        if (l - e == 3) {
            if (!memcmp(npath + e, "png", 3)) {
                attempt = IMG_PNG;
                r = sg_file_get(path, strlen(path), 0, &fbuf, maxsize, &ferr);
                if (!r) {
                    r = sg_pixbuf_loadpng(pbuf, fbuf.data, fbuf.length, err);
                    goto havefile;
                }
            } else if (!memcmp(npath + e, "jpg", 3)) {
                attempt = IMG_JPEG;
                r = sg_file_get(path, strlen(path), 0, &fbuf, maxsize, &ferr);
                if (!r) {
                    r = sg_pixbuf_loadjpeg(pbuf, fbuf.data, fbuf.length, err);
                    goto havefile;
                }
            }
        }
        if (attempt >= 0) {
            if (ferr->domain != &SG_ERROR_NOTFOUND)
                goto ferror;
            sg_error_clear(&ferr);
            e--;
        } else {
            /* Don't strip the extension if it's not a known
               extension.  */
            e = l;
        }
    }

    /* Try remaining extensions.  */
    memcpy(npath, path, e);
    if (attempt != IMG_PNG) {
        memcpy(npath + e, ".png", 5);
        r = sg_file_get(npath, strlen(npath), 0, &fbuf, maxsize, &ferr);
        if (!r) {
            r = sg_pixbuf_loadpng(pbuf, fbuf.data, fbuf.length, err);
            goto havefile;
        }
        if (ferr->domain != &SG_ERROR_NOTFOUND)
            goto ferror;
        sg_error_clear(&ferr);
    }
    if (attempt != IMG_JPEG) {
        memcpy(npath + e, ".jpg", 5);
        r = sg_file_get(npath, strlen(npath), 0, &fbuf, maxsize, &ferr);
        if (!r) {
            r = sg_pixbuf_loadpng(pbuf, fbuf.data, fbuf.length, err);
            goto havefile;
        }
        if (ferr->domain != &SG_ERROR_NOTFOUND)
            goto ferror;
        sg_error_clear(&ferr);
    }
    sg_error_notfound(err, path);
    return -1;

havefile:
    sg_buffer_destroy(&fbuf);
    return r;

ferror:
    sg_error_move(err, &ferr);
    return -1;
}
