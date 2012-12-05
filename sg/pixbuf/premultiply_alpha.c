/* Copyright 2012 Dietrich Epp <depp@zdome.net> */
#include "sg/pixbuf.h"

static void
sg_pixbuf_premultiply_ya(unsigned char *ptr, size_t npix)
{
    size_t i;
    unsigned a, y;
    for (i = 0; i < npix; ++i) {
        y = ptr[i*2+0];
        a = ptr[i*2+1];
        y = y * a;
        y = (y + 1 + (y >> 8)) >> 8;
        ptr[i*2+0] = y;
    }
}

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
    size_t npix = (size_t) pbuf->pwidth * (size_t) pbuf->pheight;
    switch (pbuf->format) {
    case SG_YA:
        sg_pixbuf_premultiply_ya(pbuf->data, npix);
        break;

    case SG_RGBA:
        sg_pixbuf_premultiply_rgba(pbuf->data, npix);
        break;

    default:
        break;
    }
}
