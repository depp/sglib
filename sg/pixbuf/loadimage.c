/* Copyright 2012 Dietrich Epp <depp@zdome.net> */
/* Maximum size of an image file in bytes.  */
#define IMAGE_MAXSIZE (16 * 1024 * 1024)

#include "sg/error.h"
#include "sg/file.h"
#include "sg/pixbuf.h"
#include <string.h>

const char SG_PIXBUF_IMAGE_EXTENSIONS[] = "png:jpg";

static const unsigned char SG_PIXBUF_PNGHEAD[8] = {
    137, 80, 78, 71, 13, 10, 26, 10
};

static const unsigned char SG_PIXBUF_JPEGHEAD[2] = { 255, 216 };

int
sg_pixbuf_loadimage(struct sg_pixbuf *pbuf, const void *data, size_t len,
                    struct sg_error **err)
{
    if (len >= 8 && !memcmp(data, SG_PIXBUF_PNGHEAD, 8))
        return sg_pixbuf_loadpng(pbuf, data, len, err);
    if (len >= 2 && !memcmp(data, SG_PIXBUF_JPEGHEAD, 2))
        return sg_pixbuf_loadjpeg(pbuf, data, len, err);
    sg_error_data(err, "image");
    return -1;
}
