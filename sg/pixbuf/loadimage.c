/* Maximum size of an image file in bytes.  */
#define IMAGE_MAXSIZE (16 * 1024 * 1024)

#include "error.h"
#include "file.h"
#include "pixbuf.h"
#include <string.h>

static const unsigned char SG_PIXBUF_PNGHEAD[8] = {
    137, 80, 78, 71, 13, 10, 26, 10
};

static const unsigned char SG_PIXBUF_JPEGHEAD[2] = { 255, 216 };

int
sg_pixbuf_loadimage(struct sg_pixbuf *pbuf, const char *path,
                    struct sg_error **err)
{
    struct sg_buffer fbuf;
    size_t len;
    int r;
    const unsigned char *p;

    r = sg_file_get(path, strlen(path), 0, "png:jpg", &fbuf,
                    IMAGE_MAXSIZE, err);
    if (r)
        return -1;

    p = fbuf.data;
    len = fbuf.length;
    if (len >= 8 && !memcmp(p, SG_PIXBUF_PNGHEAD, 8)) {
        r = sg_pixbuf_loadpng(pbuf, p, len, err);
        goto done;
    }
    if (len >= 2 && !memcmp(p, SG_PIXBUF_JPEGHEAD, 2)) {
        r = sg_pixbuf_loadjpeg(pbuf, p, len, err);
        goto done;
    }
    r = -1;
    sg_error_data(err, "image");
    goto done;

done:
    sg_buffer_destroy(&fbuf);
    return r;
}
