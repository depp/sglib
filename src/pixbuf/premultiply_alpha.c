/* Copyright 2012-2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "sg/pixbuf.h"
#include "private.h"

static void
sg_pixbuf_premultiply_rgba(unsigned char *ptr, size_t npix)
{
    size_t i;
    unsigned a, r, g, b;
    for (i = 0; i < npix; ++i) {
        r = ptr[i*4+0];
        g = ptr[i*4+1];
        b = ptr[i*4+2];
        a = ptr[i*4+3];
        r = r * a;
        r = (r + 1 + (r >> 8)) >> 8;
        g = g * a;
        g = (g + 1 + (g >> 8)) >> 8;
        b = b * a;
        b = (b + 1 + (b >> 8)) >> 8;
        ptr[i*4+0] = r;
        ptr[i*4+1] = g;
        ptr[i*4+2] = b;
    }
}

void
sg_pixbuf_premultiply_alpha(struct sg_pixbuf *pbuf)
{
    size_t npix = (size_t) pbuf->width * (size_t) pbuf->height;
    switch (pbuf->format) {
    case SG_RGBA:
        sg_pixbuf_premultiply_rgba(pbuf->data, npix);
        break;

    default:
        break;
    }
}
