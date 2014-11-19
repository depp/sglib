/* Copyright 2012-2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "sg/error.h"
#include "sg/file.h"
#include "sg/pixbuf.h"
#include "private.h"

#include <ApplicationServices/ApplicationServices.h>
#include <assert.h>

/* Core Graphics does not tell us what is wrong when it fails.
   http://www.red-sweater.com/blog/129/coregraphics-log-jam */

#define FAIL() do { lineno = __LINE__; goto error; } while (0)

struct sg_image_cg {
    struct sg_image img;
    CGImageRef cgimg;
};

static void
sg_image_cg_freedata(void *info, const void *data, size_t size)
{
    (void) data;
    (void) size;
    sg_filedata_decref(info);
}

static CGDataProviderRef
sg_image_cg_data(struct sg_filedata *data)
{
    CGDataProviderRef p = CGDataProviderCreateWithData(
        data, data->data, data->length, sg_image_cg_freedata);
    if (!p)
        abort();
    sg_filedata_incref(data);
    return p;
}

static void
sg_image_cg_free(struct sg_image *img)
{
    struct sg_image_cg *im = (struct sg_image_cg *) img;
    CGImageRelease(im->cgimg);
    free(im);
}

static int
sg_image_cg_draw(struct sg_image *img, struct sg_pixbuf *pbuf,
                 int x, int y, struct sg_error **err)
{
    int lineno;
    struct sg_image_cg *im = (struct sg_image_cg *) img;
    int iw = im->img.width, ih = im->img.height,
        pw = pbuf->width, ph = pbuf->height, rb = pbuf->rowbytes;
    CGColorSpaceRef color_space = NULL;
    CGContextRef cxt = NULL;

    color_space = CGColorSpaceCreateDeviceRGB();
    if (!color_space)
        FAIL();
    cxt = CGBitmapContextCreate(
        pbuf->data, pw, ph, 8, rb,
        color_space, kCGImageAlphaPremultipliedLast);
    if (!cxt)
        FAIL();
    CGContextSetBlendMode(cxt, kCGBlendModeCopy);
    CGContextDrawImage(
        cxt, CGRectMake(0, ph - ih, iw, ih), im->cgimg);

    CGContextRelease(cxt);
    CGColorSpaceRelease(color_space);
    return 0;

error:
    if (cxt) CGContextRelease(cxt);
    if (color_space) CGColorSpaceRelease(color_space);
    sg_error_setf(err, &SG_ERROR_GENERIC, 0,
                  "could not read image (%s: %d)", __FUNCTION__, lineno);
    return -1;
}

static struct sg_image *
sg_image_cg_new(CGImageRef img, struct sg_error **err)
{
    struct sg_image_cg *im = malloc(sizeof(*im));
    if (!img) {
        CGImageRelease(img);
        sg_error_nomem(err);
        return NULL;
    }

    size_t w = CGImageGetWidth(img), h = CGImageGetHeight(img);
    if (w > INT_MAX || h > INT_MAX)
        goto invalid;

    unsigned flags = 0;
    switch (CGImageGetAlphaInfo(img)) {
    case kCGImageAlphaPremultipliedLast:
    case kCGImageAlphaPremultipliedFirst:
    case kCGImageAlphaLast:
    case kCGImageAlphaFirst:
    case kCGImageAlphaOnly:
        flags |= SG_IMAGE_ALPHA;
        break;
    default:
        break;
    }
    CGColorSpaceRef color_space = CGImageGetColorSpace(img);
    if (CGColorSpaceGetModel(color_space) != kCGColorSpaceModelMonochrome)
        flags |= SG_IMAGE_COLOR;

    im->img.width = (int) w;
    im->img.height = (int) h;
    im->img.flags = flags;
    im->img.free = sg_image_cg_free;
    im->img.draw = sg_image_cg_draw;
    im->cgimg = img;
    return &im->img;

invalid:
    sg_error_data(err, "image");
    free(im);
    CGImageRelease(img);
    return NULL;
}

#if defined ENABLE_PNG_COREGRAPHICS

struct sg_image *
sg_image_png(struct sg_filedata *data, struct sg_error **err)
{
    CGDataProviderRef dp = sg_image_cg_data(data);
    CGImageRef img = CGImageCreateWithPNGDataProvider(
        dp, NULL, false, kCGRenderingIntentDefault);
    CGDataProviderRelease(dp);
    return sg_image_cg_new(img, err);
}

#endif

#if defined ENABLE_JPEG_COREGRAPHICS

struct sg_image *
sg_image_jpeg(struct sg_filedata *data, struct sg_error **err)
{
    CGDataProviderRef dp = sg_image_cg_data(data);
    CGImageRef img = CGImageCreateWithJPEGDataProvider(
        dp, NULL, false, kCGRenderingIntentDefault);
    CGDataProviderRelease(dp);
    return sg_image_cg_new(img, err);
}

#endif

static void
sg_image_cg_freedata2(void *info, const void *data, size_t size)
{
    (void) data;
    (void) size;
    (void) info;
}

struct sg_coregraphics_consumer {
    struct sg_error *err;
    struct sg_writer *fp;
};

static size_t
sg_pixbuf_cg_iowrite(void *info, const void *buffer, size_t count)
{
    struct sg_coregraphics_consumer *cp = info;
    int r = sg_writer_write(cp->fp, buffer, count, &cp->err);
    return r < 0 ? 0 : (size_t) r;
}

static void
sg_pixbuf_cg_iorelease(void *info)
{
    (void) info;
}

int
sg_pixbuf_writepng(struct sg_pixbuf *pbuf, const char *path, size_t pathlen,
                   struct sg_error **err)
{
    int ret, nchan, lineno, r;
    bool success;
    CGBitmapInfo info;
    CGDataConsumerCallbacks cb;
    CGColorSpaceRef color_space = NULL;
    CGDataProviderRef data = NULL;
    CGDataConsumerRef consumer = NULL;
    CGImageDestinationRef dest = NULL;
    CGImageRef img;
    struct sg_writer *fp;
    struct sg_coregraphics_consumer cdata;

    cdata.err = NULL;

    switch (pbuf->format) {
    case SG_RGBX: nchan = 4; info = kCGImageAlphaNoneSkipLast; break;
    case SG_RGBA: nchan = 4; info = kCGImageAlphaLast; break;
    default:
        sg_error_invalid(err, __FUNCTION__, "pbuf");
        return -1;
    }

    fp = sg_writer_open(path, pathlen, err);
    if (!fp)
        return -1;

    color_space = CGColorSpaceCreateDeviceRGB();
    if (!color_space)
        FAIL();

    data = CGDataProviderCreateWithData(
        NULL, pbuf->data, (size_t) pbuf->rowbytes * pbuf->height,
        sg_image_cg_freedata2);
    if (!data)
        FAIL();

    img = CGImageCreate(
        pbuf->width, pbuf->height,
        8, nchan * 8, pbuf->rowbytes,
        color_space, info, data, NULL, false, kCGRenderingIntentDefault);
    if (!img)
        FAIL();

    cdata.fp = fp;
    cb.putBytes = sg_pixbuf_cg_iowrite;
    cb.releaseConsumer = sg_pixbuf_cg_iorelease;
    consumer = CGDataConsumerCreate(&cdata, &cb);

    dest = CGImageDestinationCreateWithDataConsumer(
        consumer, kUTTypePNG, 1, NULL);
    if (!dest)
        FAIL();
    CGImageDestinationAddImage(dest, img, NULL);
    success = CGImageDestinationFinalize(dest);
    if (!success)
        FAIL();

    r = sg_writer_commit(fp, err);
    if (!r)
        goto fileerror;

    ret = 0;
    goto done;

done:
    sg_error_clear(&cdata.err);
    if (dest) CFRelease(dest);
    if (consumer) CFRelease(consumer);
    if (data) CFRelease(data);
    if (color_space) CFRelease(color_space);
    sg_writer_close(fp);
    return ret;

error:
    if (cdata.err)
        goto fileerror;
    sg_error_setf(err, &SG_ERROR_GENERIC, 0,
                  "could not write PNG (%s:%d)", __FUNCTION__, lineno);
    ret = -1;
    goto done;

fileerror:
    sg_error_move(err, &cdata.err);
    ret = -1;
    goto done;
}
