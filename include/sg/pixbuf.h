/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#ifndef SG_PIXBUF_H
#define SG_PIXBUF_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
struct sg_error;
struct sg_file;

/* Arbitrary limit on image file size.  TODO: Make this
   configurable.  */
#define SG_PIXBUF_MAXFILESZ ((size_t) 1024 * 1024 * 8)

/* A list of supported image extensions, without the leading `'.'`,
   and separated by `':'`.  */
extern const char SG_PIXBUF_IMAGE_EXTENSIONS[];

typedef enum {
    /* 8-bit formats.  Y means grayscale.  All formats with alpha are
       assumed to use premultiplied alpha unless otherwise
       specified.  */
    SG_Y,
    SG_YA,
    SG_RGB,
    SG_RGBX,
    SG_RGBA
} sg_pixbuf_format_t;

#define SG_PIXBUF_NFORMAT ((int) SG_RGBA + 1)

struct sg_pixbuf {
    void *data;
    sg_pixbuf_format_t format;
    /* Dimensions of the image */
    int iwidth;
    int iheight;
    /* Dimensions of the buffer */
    int pwidth;
    int pheight;
    int rowbytes;
};

void
sg_pixbuf_init(struct sg_pixbuf *pbuf);

void
sg_pixbuf_destroy(struct sg_pixbuf *pbuf);

/* Set the dimensions and format of the pixel buffer.  The dimensions
   and format will be altered for optimal driver support.  For
   example, the width and height may be rounded up to a power of two,
   and the format may be changed.  Returns 0 if successful, -1 if the
   requested format is unsupported.  */
int
sg_pixbuf_set(struct sg_pixbuf *pbuf, sg_pixbuf_format_t format,
              int width, int height, struct sg_error **err);

/* Allocate memory for the pixel buffer.  Returns 0 on success, -1 on
   failure.  */
int
sg_pixbuf_alloc(struct sg_pixbuf *pbuf, struct sg_error **err);

/* Allocate zeroed memory for the pixel buffer.  Returns 0 on success,
   -1 on failure.  */
int
sg_pixbuf_calloc(struct sg_pixbuf *pbuf, struct sg_error **err);

/* Load an image from the given buffer.  The image format will be
   automatically detected.  */
int
sg_pixbuf_loadimage(struct sg_pixbuf *pbuf, const void *data, size_t len,
                    struct sg_error **err);

/* Load a PNG image from the given buffer.  */
int
sg_pixbuf_loadpng(struct sg_pixbuf *pbuf, const void *data, size_t len,
                  struct sg_error **err);

/* Write a PNG image to the given file.  */
int
sg_pixbuf_writepng(struct sg_pixbuf *pbuf, struct sg_file *fp,
                   struct sg_error **err);

/* Load a JPEG image from the given buffer.  */
int
sg_pixbuf_loadjpeg(struct sg_pixbuf *pbuf, const void *data, size_t len,
                   struct sg_error **err);

/* Convert the alpha channel in the pixel buffer to a premultiplied
   alpha channel.  */
void
sg_pixbuf_premultiply_alpha(struct sg_pixbuf *pbuf);

#ifdef __cplusplus
}
#endif
#endif
