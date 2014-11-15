/* Copyright 2012-2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
/* <stdio.h> is necessary for <jpeglib.h> */

#include "sg/error.h"
#include "sg/file.h"
#include "sg/log.h"
#include "sg/pixbuf.h"
#include "sg/version.h"
#include "private.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <jpeglib.h>

struct sg_image_jpeg {
    struct sg_image img;

    struct sg_buffer *buf;
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    struct jpeg_source_mgr jsrc;
};

/* These few methods for the input source are basically lifted from
   the LibJPEG source code, for a *later version* with a "memory
   source".  */

static void
sg_jpeg_initsource(j_decompress_ptr cinfo)
{
    (void) cinfo;
}

static void
sg_jpeg_termsource(j_decompress_ptr cinfo)
{
    (void) cinfo;
}

static boolean
sg_jpeg_fillinput(j_decompress_ptr cinfo)
{
    static JOCTET buf[4];
    /* WARN */
    buf[0] = 0xff;
    buf[1] = JPEG_EOI;
    cinfo->src->next_input_byte = buf;
    cinfo->src->bytes_in_buffer = 2;
    return TRUE;
}

static void
sg_jpeg_skip(j_decompress_ptr cinfo, long num_bytes)
{
    struct jpeg_source_mgr *src = cinfo->src;
    if (num_bytes <= 0)
        return;
    if ((size_t) num_bytes > src->bytes_in_buffer) {
        sg_jpeg_fillinput(cinfo);
    } else {
        src->next_input_byte += num_bytes;
        src->bytes_in_buffer -= num_bytes;
    }
}

static void
sg_image_jpeg_free(struct sg_image *img)
{
    struct sg_image_jpeg *im = (struct sg_image_jpeg *) img;
    sg_buffer_decref(im->buf);
    jpeg_destroy_decompress(&im->cinfo);
    free(im);
}

static int
sg_image_jpeg_draw(struct sg_image *img, struct sg_pixbuf *pbuf,
                   int x, int y, struct sg_error **err)
{
    struct sg_image_jpeg *im = (struct sg_image_jpeg *) img;
    int iw = im->img.width, ih = im->img.height,
        pw = pbuf->width, ph = pbuf->height, rb = pbuf->rowbytes;
    unsigned char *data = pbuf->data, *rowp[1];
    if ((pbuf->format != SG_RGBX && pbuf->format != SG_RGBA) ||
        x < 0 || iw > pw || x > pw - iw ||
        y < 0 || ih > ph || y > ph - ih ||
        !data)
        goto invalid;

    jpeg_start_decompress(&im->cinfo);
    while (im->cinfo.output_scanline < (unsigned) ih) {
        rowp[0] = data + (im->cinfo.output_scanline + y) * rb + x * 4;
        jpeg_read_scanlines(&im->cinfo, rowp, 1);
    }
    jpeg_finish_decompress(&im->cinfo);

    return 0;

invalid:
    sg_error_invalid(err, __FUNCTION__, "pbuf");
    return -1;
}

struct sg_image *
sg_image_jpeg(struct sg_buffer *buf, struct sg_error **err)
{
    struct sg_image_jpeg *im;

    im = malloc(sizeof(*im));
    if (!im) {
        sg_error_nomem(err);
        return NULL;
    }

    jpeg_create_decompress(&im->cinfo);

    /* FIXME: this aborts.  */
    im->cinfo.err = jpeg_std_error(&im->jerr);
    im->cinfo.src = &im->jsrc;

    im->jsrc.next_input_byte = buf->data;
    im->jsrc.bytes_in_buffer = buf->length;
    im->jsrc.init_source = sg_jpeg_initsource;
    im->jsrc.fill_input_buffer = sg_jpeg_fillinput;
    im->jsrc.skip_input_data = sg_jpeg_skip;
    im->jsrc.resync_to_restart = jpeg_resync_to_restart;
    im->jsrc.term_source = sg_jpeg_termsource;

    jpeg_read_header(&im->cinfo, TRUE);
    if (im->cinfo.image_width > INT_MAX ||
        im->cinfo.image_height > INT_MAX)
        abort();
    /* FIXME: require libjpeg-turbo */
    im->cinfo.out_color_space = JCS_EXT_RGBA;

    im->img.width = (int) im->cinfo.image_width;
    im->img.height = (int) im->cinfo.image_height;
    im->img.flags =
        (im->cinfo.jpeg_color_space != JCS_GRAYSCALE ?
         SG_IMAGE_COLOR : 0);
    im->img.free = sg_image_jpeg_free;
    im->img.draw = sg_image_jpeg_draw;

    im->buf = buf;
    sg_buffer_incref(buf);

    return &im->img;
}

/*
sg_error_data(err, "JPEG");
sg_logf(sg_logger_get("image"), SG_LOG_ERROR,
"JPEG has unknown color space");
*/

void
sg_version_libjpeg(void)
{
    char vers[8];
    int major, minor;

#if defined JPEG_LIB_VERSION_MINOR
    major = JPEG_LIB_VERSION_MAJOR;
    minor = JPEG_LIB_VERSION_MINOR;
#else
    major = JPEG_LIB_VERSION / 10;
    minor = JPEG_LIB_VERSION % 10;
#endif

    if (minor)
        snprintf(vers, sizeof(vers), "%d%c", major, minor + 'a');
    else
        snprintf(vers, sizeof(vers), "%d", major);
    sg_version_lib("LibJPEG", vers, NULL);
}
