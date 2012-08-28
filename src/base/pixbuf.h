#ifndef BASE_PIXBUF_H
#define BASE_PIXBUF_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
struct sg_error;
struct sg_file;

typedef enum {
    /* 8-bit formats.  Y means grayscale.  All formats with alpha are
       assumed to use premultiplied alpha unless otherwise
       specified.  */
    SG_Y,
    SG_YA,
    SG_RGB,
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

/* Load an image with the given path.  This will attempt to load
   ".png" and then ".jpg".  If the path already has an extension of
   ".png" or ".jpg", that extension will be tried before the others.
   If a file is found with one of the extensions, the remaining
   extensions will not be attempted.  */
int
sg_pixbuf_loadimage(struct sg_pixbuf *pbuf, const char *path,
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
