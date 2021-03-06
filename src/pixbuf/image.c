/* Copyright 2012-2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */

/* Maximum size of an image file in bytes.  */
#define SG_IMAGE_MAXSIZE (16 * 1024 * 1024)

#include "sg/error.h"
#include "sg/file.h"
#include "sg/pixbuf.h"
#include "private.h"

#include <string.h>

const char SG_PIXBUF_IMAGE_EXTENSIONS[] = ""
#if defined ENABLE_PNG
    "png:"
#endif
#if defined ENABLE_JPEG
    "jpg:"
#endif
    ;

#if defined ENABLE_PNG
static const unsigned char SG_PIXBUF_PNGHEAD[8] = {
    137, 80, 78, 71, 13, 10, 26, 10
};
#endif

#if defined ENABLE_JPEG
static const unsigned char SG_PIXBUF_JPEGHEAD[2] = { 255, 216 };
#endif

struct sg_image *
sg_image_file(const char *path, size_t pathlen, struct sg_error **err)
{
    struct sg_filedata *data;
    struct sg_image *image;
    int r;
    r = sg_file_load(
        &data, path, pathlen,
        0, SG_PIXBUF_IMAGE_EXTENSIONS, SG_IMAGE_MAXSIZE, NULL, err);
    if (r)
        return NULL;
    image = sg_image_buffer(data, err);
    sg_filedata_decref(data);
    return image;
}

struct sg_image *
sg_image_buffer(struct sg_filedata *data, struct sg_error **err)
{
    const void *ptr = data->data;
    size_t len = data->length;

    (void) ptr;
    (void) len;

#if defined ENABLE_PNG
    if (len >= 8 && !memcmp(ptr, SG_PIXBUF_PNGHEAD, 8))
        return sg_image_png(data, err);
#endif

#if defined ENABLE_JPEG
    if (len >= 2 && !memcmp(ptr, SG_PIXBUF_JPEGHEAD, 2))
        return sg_image_jpeg(data, err);
#endif

    sg_error_data(err, "image");
    return NULL;
}

#if !defined ENABLE_PNG

int
sg_pixbuf_writepng(struct sg_pixbuf *pbuf, const char *path, size_t pathlen,
                   struct sg_error **err)
{
    (void) pbuf;
    (void) path;
    (void) pathlen;
    sg_error_disabled(err, "png");
    return -1;
}

#endif
