#include <stddef.h>
struct sg_pixbuf;
struct sg_error;

/* Load a PNG image from a memory buffer.  */
int
sg_pixbuf_loadpng(struct sg_pixbuf *pbuf, const void *data, size_t len,
                  struct sg_error **err);

/* Load a JPEG image from a memory buffer.  */
int
sg_pixbuf_loadjpeg(struct sg_pixbuf *pbuf, const void *data, size_t len,
                   struct sg_error **err);
