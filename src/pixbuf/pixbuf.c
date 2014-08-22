/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "sg/util.h"
#include "sg/error.h"
#include "sg/pixbuf.h"
#include <stdlib.h>
#include <limits.h>

#define MAX_DIM 32768

static const struct sg_error_domain SG_ERROR_PIXBUF = { "pixbuf" };

static void
sg_pixbuf_error(struct sg_error **err, const char *msg)
{
    sg_error_sets(err, &SG_ERROR_PIXBUF, 0, msg);
}

void
sg_pixbuf_init(struct sg_pixbuf *pbuf)
{
    pbuf->data = NULL;
    pbuf->format = SG_Y;
    pbuf->iwidth = 0;
    pbuf->iheight = 0;
    pbuf->pwidth = 0;
    pbuf->pheight = 0;
    pbuf->rowbytes = 0;
}

void
sg_pixbuf_destroy(struct sg_pixbuf *pbuf)
{
    if (pbuf->data)
        free(pbuf->data);
    sg_pixbuf_init(pbuf);
}

int
sg_pixbuf_set(struct sg_pixbuf *pbuf, sg_pixbuf_format_t format,
              int width, int height, struct sg_error **err)
{
    int psz, w, h, npx;
    sg_pixbuf_format_t f;

    if (width > MAX_DIM || height > MAX_DIM ||
        width < 0 || height < 0)
        goto toolarge;
    w = sg_round_up_pow2_32(width);
    h = sg_round_up_pow2_32(height);
    if (w > INT_MAX / h)
        goto toolarge;

    f = format;
    switch (format) {
    case SG_Y:
        psz = 1;
        break;

    case SG_YA:
        psz = 2;
        break;

    case SG_RGB:
        psz = 3;
        break;

    case SG_RGBX:
    case SG_RGBA:
        psz = 4;
        break;

    default:
        goto unkformat;
    }

    npx = w * h;
    if (npx > INT_MAX / psz)
        goto toolarge;

    pbuf->format = f;
    pbuf->iwidth = width;
    pbuf->iheight = height;
    pbuf->pwidth = w;
    pbuf->pheight = h;
    pbuf->rowbytes = psz * w;
    return 0;

toolarge:
    sg_pixbuf_error(err, "dimensions too large");
    return -1;

unkformat:
    sg_pixbuf_error(err, "unknown format");
    return -1;
}

int
sg_pixbuf_alloc(struct sg_pixbuf *pbuf, struct sg_error **err)
{
    if (!pbuf->pwidth || !pbuf->pheight || !pbuf->rowbytes) {
        sg_pixbuf_error(err, "can't allocate unsized image");
        return -1;
    }
    if (pbuf->data) {
        free(pbuf->data);
        pbuf->data = NULL;
    }
    pbuf->data = malloc(pbuf->rowbytes * pbuf->pheight);
    if (!pbuf->data) {
        sg_error_nomem(err);
        return -1;
    }
    return 0;
}

int
sg_pixbuf_calloc(struct sg_pixbuf *pbuf, struct sg_error **err)
{
    if (!pbuf->pwidth || !pbuf->pheight || !pbuf->rowbytes) {
        sg_pixbuf_error(err, "can't allocate unsized image");
        return -1;
    }
    if (pbuf->data) {
        free(pbuf->data);
        pbuf->data = NULL;
    }
    pbuf->data = calloc(pbuf->rowbytes * pbuf->pheight, 1);
    if (!pbuf->data) {
        sg_error_nomem(err);
        return -1;
    }
    return 0;
}
