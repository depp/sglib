/* Copyright 2012-2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */

#include "sg/error.h"
#include "sg/file.h"
#include "sg/log.h"
#include "sg/pixbuf.h"
#include "sg/version.h"
#include "private.h"

#include <assert.h>
#include <png.h>
#include <stdlib.h>

struct sg_image_png {
    struct sg_image img;

    png_struct *pngp;
    png_info *infop;
    struct sg_error **err;
    struct sg_buffer *buf;
    const char *bufptr, *bufend;
};

static void
sg_png_error(png_struct *pngp, const char *msg)
{
    struct sg_image_png *im = png_get_error_ptr(pngp);
    sg_logf(SG_LOG_ERROR, "LibPNG: %s", msg);
    sg_error_data(im->err, "PNG");
    longjmp(png_jmpbuf(pngp), 1);
}

static void
sg_png_warning(png_struct *pngp, const char *msg)
{
    (void) pngp;
    sg_logf(SG_LOG_WARN, "LibPNG: %s", msg);
}

static void
sg_image_png_read(png_struct *pngp, unsigned char *ptr, png_size_t len)
{
    struct sg_error **err;
    struct sg_image_png *im;
    size_t rem;
    im = png_get_io_ptr(pngp);
    rem = im->bufend - im->bufptr;
    if (len <= rem) {
        memcpy(ptr, im->bufptr, len);
        im->bufptr += len;
    } else {
        sg_logs(SG_LOG_ERROR, "PNG: unexpected end of file");
        err = png_get_error_ptr(pngp);
        sg_error_data(err, "PNG");
        longjmp(png_jmpbuf(pngp), 1);
    }
}

static void
sg_image_png_free(struct sg_image *img)
{
    struct sg_image_png *im = (struct sg_image_png *) img;
    png_destroy_read_struct(&im->pngp, &im->infop, NULL);
    sg_buffer_decref(im->buf);
    free(im);
}

static int
sg_image_png_draw(struct sg_image *img, struct sg_pixbuf *pbuf,
                  int x, int y, struct sg_error **err)
{
    struct sg_image_png *im = (struct sg_image_png *) img;
    int iw = im->img.width, ih = im->img.height,
        pw = pbuf->width, ph = pbuf->height, rb = pbuf->rowbytes, i;
    unsigned char *data = pbuf->data;
    png_struct *pngp = im->pngp;
    png_byte **rowp;
    void *volatile mem;
    struct sg_pixbuf rect;

    if ((pbuf->format != SG_RGBX && pbuf->format != SG_RGBA) ||
        x < 0 || iw > pw || x > pw - iw ||
        y < 0 || ih > ph || y > ph - ih ||
        !data) {
        sg_error_invalid(err, __FUNCTION__, "pbuf");
        return -1;
    }

    rowp = malloc(sizeof(*rowp) * ih);
    if (!rowp) {
        sg_error_nomem(err);
        return -1;
    }
    mem = rowp;

    im->err = err;
    if (setjmp(png_jmpbuf(pngp))) {
        free(mem);
        return -1;
    }

    for (i = 0; i < ih; ++i)
        rowp[i] = data + rb * (i + y) + x * 4;
    png_read_image(im->pngp, rowp);
    png_read_end(im->pngp, NULL);

    if ((im->img.flags & SG_IMAGE_ALPHA) != 0 &&
        pbuf->format == SG_RGBA) {
        rect.data = data + rb * y + x;
        rect.format = SG_RGBA;
        rect.width = iw;
        rect.height = ih;
        rect.rowbytes = rb;
        sg_pixbuf_premultiply_alpha(&rect);
    }

    im->err = NULL;
    free(rowp);
    return 0;
}

struct sg_image *
sg_image_png(struct sg_buffer *buf, struct sg_error **err)
{
    png_struct *pngp;
    png_info *infop;
    struct sg_image_png *im;
    png_uint_32 width, height;
    int depth, ctype, has_trns;
    const char *msg;

    im = malloc(sizeof(*im));
    if (!im) {
        sg_error_nomem(err);
        return NULL;
    }

    im->pngp = pngp = png_create_read_struct_2(
        PNG_LIBPNG_VER_STRING,
        im, sg_png_error, sg_png_warning,
        NULL, NULL, NULL);
    if (!pngp) {
        sg_error_nomem(err);
        free(im);
        return NULL;
    }

    im->infop = infop = png_create_info_struct(pngp);
    if (!infop) {
        sg_error_nomem(err);
        goto cleanup;
    }

    im->err = err;
    im->buf = buf;
    im->bufptr = buf->data;
    im->bufend = (const char *) buf->data + buf->length;

    if (setjmp(png_jmpbuf(pngp)))
        goto cleanup;

    png_set_read_fn(pngp, im, sg_image_png_read);
    png_read_info(pngp, infop);
    png_get_IHDR(pngp, infop, &width, &height, &depth, &ctype,
                 NULL, NULL, NULL);
    if (width > INT_MAX || height > INT_MAX) {
        msg = "PNG image is too large";
        goto invalid;
    }

    has_trns = png_get_valid(pngp, infop, PNG_INFO_tRNS) != 0;
    png_set_expand(pngp);
    png_set_strip_16(pngp);
    png_set_gray_to_rgb(pngp);
    png_set_add_alpha(pngp, 255, PNG_FILLER_AFTER);

    im->img.width = (int) width;
    im->img.height = (int) height;
    im->img.flags =
        (has_trns || (ctype & PNG_COLOR_MASK_ALPHA) != 0 ?
         SG_IMAGE_ALPHA : 0) |
        ((ctype & (PNG_COLOR_MASK_PALETTE | PNG_COLOR_MASK_COLOR)) != 0 ?
         SG_IMAGE_COLOR : 0);
    im->img.free = sg_image_png_free;
    im->img.draw = sg_image_png_draw;
    sg_buffer_incref(buf);
    im->err = NULL;

    return &im->img;

invalid:
    sg_error_data(err, "PNG");
    sg_logf(SG_LOG_ERROR, msg);
    goto cleanup;

cleanup:
    png_destroy_read_struct(&im->pngp, &im->infop, NULL);
    free(im);
    return NULL;
}

static void
sg_png_write(png_structp pngp, png_bytep data, png_size_t length)
{
    struct sg_error **err;
    struct sg_file *fp = png_get_io_ptr(pngp);
    int r;
    png_size_t pos = 0;

again:
    r = fp->write(fp, data, length);
    if (r > 0) {
        pos += r;
        if (pos < length)
            goto again;
    } else {
        err = png_get_error_ptr(pngp);
        sg_error_move(err, &fp->err);
        longjmp(png_jmpbuf(pngp), 1);
    }
}

static void
sg_png_flush(png_structp pngp)
{
    (void) pngp;
}

int
sg_pixbuf_writepng(struct sg_pixbuf *pbuf, const char *path, size_t pathlen,
                   struct sg_error **err)
{
    struct sg_file *fp;
    volatile png_structp pngp = NULL;
    volatile png_infop infop = NULL;
    int ret, ctype, i, r;
    png_bytep *rowp;
    void *volatile tmp = NULL;

    switch (pbuf->format) {
    case SG_RGBX: ctype = PNG_COLOR_TYPE_RGB; break;
    case SG_RGBA: ctype = PNG_COLOR_TYPE_RGB_ALPHA; break;
    default:
        sg_error_invalid(err, __FUNCTION__, "pbuf");
        return -1;
    }

    fp = sg_file_open(path, pathlen, SG_WRONLY, NULL, err);
    if (!fp)
        return -1;

    pngp = png_create_write_struct(
        PNG_LIBPNG_VER_STRING,
        err, sg_png_error, sg_png_warning);
    if (!pngp) {
        ret = -1;
        goto done;
    }

    infop = png_create_info_struct(pngp);
    if (!infop) {
        ret = -1;
        goto done;
    }

    if (setjmp(png_jmpbuf(pngp))) {
        ret = -1;
        goto done;
    }

    png_set_write_fn(pngp, fp, sg_png_write, sg_png_flush);

    png_set_IHDR(
        pngp, infop,
        pbuf->width, pbuf->height, 8, ctype,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT,
        PNG_FILTER_TYPE_DEFAULT);

    png_write_info(pngp, infop);

    if (pbuf->format == SG_RGBX)
        png_set_filler(pngp, 0, PNG_FILLER_AFTER);

    rowp = malloc(sizeof(*rowp) * pbuf->height);
    if (!rowp) {
        sg_error_nomem(err);
        ret = -1;
        goto done;
    }
    tmp = rowp;
    for (i = 0; i < pbuf->height; ++i)
        rowp[i] = (unsigned char *) pbuf->data + i * pbuf->rowbytes;
    png_write_image(pngp, rowp);
    png_write_end(pngp, NULL);

    ret = 0;
    goto done;

done:
    if (!ret) {
        r = fp->commit(fp);
        if (r) {
            ret = -1;
            sg_error_move(err, &fp->err);
        }
    }
    fp->close(fp);
    free(tmp);
    png_destroy_write_struct(
        (png_structp *) &pngp, (png_infop *) &infop);
    return ret;
}

void
sg_version_libpng(void)
{
    int v = png_access_version_number(), maj, min, mic;
    char vers[16];
    min = v / 100;
    mic = v % 100;
    maj = min / 100;
    min = min % 100;
    snprintf(vers, sizeof(vers), "%d.%d.%d", maj, min, mic);
    sg_version_lib("LibPNG", PNG_LIBPNG_VER_STRING, vers);
}
