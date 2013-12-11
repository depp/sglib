/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
/* <stdio.h> is necessary for <jpeglib.h> */

#include "sg/error.h"
#include "sg/log.h"
#include "sg/pixbuf.h"
#include "sg/version.h"
#include <stdio.h>
#include <jpeglib.h>

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
    if (num_bytes > src->bytes_in_buffer) {
        sg_jpeg_fillinput(cinfo);
    } else {
        src->next_input_byte += num_bytes;
        src->bytes_in_buffer -= num_bytes;
    }
}

int
sg_pixbuf_loadjpeg(struct sg_pixbuf *pbuf, const void *data, size_t len,
                   struct sg_error **err)
{
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    struct jpeg_source_mgr jsrc;
    unsigned width, height, rowbytes;
    sg_pixbuf_format_t pfmt;
    unsigned char *pdata, *rowp[1];
    int ret, r;

    jpeg_create_decompress(&cinfo);

    /* FIXME: this aborts.  */
    cinfo.err = jpeg_std_error(&jerr);
    cinfo.src = &jsrc;

    jsrc.next_input_byte = data;
    jsrc.bytes_in_buffer = len;
    jsrc.init_source = sg_jpeg_initsource;
    jsrc.fill_input_buffer = sg_jpeg_fillinput;
    jsrc.skip_input_data = sg_jpeg_skip;
    jsrc.resync_to_restart = jpeg_resync_to_restart;
    jsrc.term_source = sg_jpeg_termsource;

    jpeg_read_header(&cinfo, TRUE);

    width = cinfo.image_width;
    height = cinfo.image_height;

    switch (cinfo.out_color_space) {
    case JCS_GRAYSCALE:
        pfmt = SG_Y;
        break;

    case JCS_RGB:
        pfmt = SG_RGB;
        break;

    default:
        sg_error_data(err, "JPEG");
        sg_logf(sg_logger_get("image"), LOG_ERROR,
                "JPEG has unknown color space");
        goto error;
    }

    r = sg_pixbuf_set(pbuf, pfmt, width, height, err);
    if (r)
        goto error;
    r = sg_pixbuf_alloc(pbuf, err);
    if (r)
        goto error;

    jpeg_start_decompress(&cinfo);

    rowbytes = pbuf->rowbytes;
    pdata = pbuf->data;
    while (cinfo.output_scanline < height) {
        rowp[0] = pdata + cinfo.output_scanline * rowbytes;
        jpeg_read_scanlines(&cinfo, rowp, 1);
    }

    jpeg_finish_decompress(&cinfo);
    ret = 0;
    goto done;

error:
    ret = -1;
    goto done;

done:
    jpeg_destroy_decompress(&cinfo);
    return ret;
}

void
sg_version_libjpeg(struct sg_logger *lp)
{
    char vers[8];
    int major, minor;
#ifndef JPEG_LIB_VERSION_MINOR
    major = JPEG_LIB_VERSION / 10;
    minor = JPEG_LIB_VERSION % 10;
#else
    major = JPEG_LIB_VERSION_MAJOR;
    minor = JPEG_LIB_VERSION_MINOR;
#endif
    if (minor)
        snprintf(vers, sizeof(vers), "%d%c", major, minor + 'a');
    else
        snprintf(vers, sizeof(vers), "%d", major);
    sg_version_lib(lp, "LibJPEG", vers, NULL);
}
