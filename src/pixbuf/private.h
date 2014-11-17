/* Copyright 2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "config.h"
#include <stddef.h>
struct sg_filedata;
struct sg_error;
struct sg_image;

#if defined ENABLE_PNG

/* Load a PNG image.  */
struct sg_image *
sg_image_png(struct sg_filedata *data, struct sg_error **err);

#endif

#if defined ENABLE_JPEG

/* Load a JPEG image.  */
struct sg_image *
sg_image_jpeg(struct sg_filedata *data, struct sg_error **err);

#endif

/* Convert image data from non-premultiplied alpha to premultiplied
   alpha.  This only works on RGBA images.  */
void
sg_pixbuf_premultiply_alpha(struct sg_pixbuf *pbuf);
