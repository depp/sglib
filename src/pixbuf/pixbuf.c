/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "sg/util.h"
#include "sg/error.h"
#include "sg/pixbuf.h"
#include <stdlib.h>
#include <limits.h>

const size_t SG_PIXBUF_FORMATSIZE[SG_PIXBUF_NFORMAT] = {
    1, 2, 4, 4
};

const char SG_PIXBUF_FORMATNAME[SG_PIXBUF_NFORMAT][5] = {
    "R", "RG", "RGBX", "RGBA"
};

#define MAX_DIM 32768

static const struct sg_error_domain SG_ERROR_PIXBUF = { "pixbuf" };

static void
sg_pixbuf_error(struct sg_error **err, const char *msg)
{
    sg_error_sets(err, &SG_ERROR_PIXBUF, 0, msg);
}

static size_t
sg_pixbuf_set(struct sg_pixbuf *pbuf, sg_pixbuf_format_t format,
              int width, int height, struct sg_error **err)
{
    int psz, w, h, npx;

    if (width <= 0 || height <= 0)
        goto invalid;
    if (width > MAX_DIM || height > MAX_DIM)
        goto toolarge;
    w = sg_round_up_pow2_32(width);
    h = sg_round_up_pow2_32(height);
    if (w > INT_MAX / h)
        goto toolarge;

    psz = SG_PIXBUF_FORMATSIZE[format];
    npx = w * h;
    if (npx > INT_MAX / psz)
        goto toolarge;

    pbuf->format = format;
    pbuf->width = width;
    pbuf->height = height;
    pbuf->rowbytes = psz * w;
    return psz * w * h;

toolarge:
    sg_pixbuf_error(err, "dimensions too large");
    return 0;

invalid:
    sg_error_invalid(err, __FUNCTION__, NULL);
    return 0;
}

int
sg_pixbuf_alloc(struct sg_pixbuf *pbuf, sg_pixbuf_format_t format,
                int width, int height, struct sg_error **err)
{
    size_t sz;
    void *ptr;
    sz = sg_pixbuf_set(pbuf, format, width, height, err);
    if (!sz)
        return -1;
    ptr = malloc(sz);
    if (!ptr) {
        sg_error_nomem(err);
        return -1;
    }
    pbuf->data = ptr;
    return 0;
}

int
sg_pixbuf_calloc(struct sg_pixbuf *pbuf, sg_pixbuf_format_t format,
                 int width, int height, struct sg_error **err)
{
    size_t sz;
    void *ptr;
    sz = sg_pixbuf_set(pbuf, format, width, height, err);
    if (!sz)
        return -1;
    ptr = calloc(sz, 1);
    if (!ptr) {
        sg_error_nomem(err);
        return -1;
    }
    pbuf->data = ptr;
    return 0;
}
