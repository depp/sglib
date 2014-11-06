/* Copyright 2012-2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "sg/error.h"
#include "sg/file.h"
#include "sg/pixbuf.h"
#include "private.h"

#include <ApplicationServices/ApplicationServices.h>
#include <assert.h>

struct sg_image_cg {
    struct sg_image img;
    CGImageRef cgimg;
};

static void sg_image_cg_freedata(void *info, const void *data, size_t size)
{
    struct sg_buffer *buf;
    (void) data;
    (void) size;
    buf = info;
    sg_buffer_decref(buf);
}

static CGDataProviderRef
sg_image_cg_data(struct sg_buffer *buf)
{
    CGDataProviderRef p = CGDataProviderCreateWithData(
        buf, buf->data, buf->length, sg_image_cg_freedata);
    if (!p)
        abort();
    sg_buffer_incref(buf);
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
    struct sg_image_cg *im = (struct sg_image_cg *) img;
    int iw = im->img.width, ih = im->img.height,
        pw = pbuf->width, ph = pbuf->height, rb = pbuf->rowbytes;
    CGColorSpaceRef color_space = CGColorSpaceCreateDeviceRGB();
    if (!color_space)
        abort();
    CGContextRef cxt = CGBitmapContextCreate(
        pbuf->data, pw, ph, 8, rb,
        color_space, kCGImageAlphaPremultipliedLast);
    if (!cxt)
        abort();
    CGColorSpaceRelease(color_space);

    CGContextSetBlendMode(cxt, kCGBlendModeCopy);
    CGContextDrawImage(
        cxt, CGRectMake(0, ph - ih, iw, ih), im->cgimg);
    CGContextRelease(cxt);

    return 0;
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
sg_image_png(struct sg_buffer *buf, struct sg_error **err)
{
    CGDataProviderRef dp = sg_image_cg_data(buf);
    CGImageRef img = CGImageCreateWithPNGDataProvider(
        dp, NULL, false, kCGRenderingIntentDefault);
    CGDataProviderRelease(dp);
    return sg_image_cg_new(img, err);
}

#endif

#if defined ENABLE_JPEG_COREGRAPHICS

struct sg_image *
sg_image_jpeg(struct sg_buffer *buf, struct sg_error **err)
{
    CGDataProviderRef dp = sg_image_cg_data(buf);
    CGImageRef img = CGImageCreateWithJPEGDataProvider(
        dp, NULL, false, kCGRenderingIntentDefault);
    CGDataProviderRelease(dp);
    return sg_image_cg_new(img, err);
}

#endif

static void sg_image_cg_freedata2(void *info, const void *data, size_t size)
{
    (void) data;
    (void) size;
    (void) info;
}

static size_t
sg_pixbuf_cg_iowrite(void *info, const void *buffer, size_t count)
{
    struct sg_file *fp = info;
    int r = fp->write(fp, buffer, count);
    return r < 0 ? 0 : (size_t) r;
}

static void
sg_pixbuf_cg_iorelease(void *info)
{
    (void) info;
}

int
sg_pixbuf_writepng(struct sg_pixbuf *pbuf, struct sg_file *fp,
                   struct sg_error **err)
{
    (void) err;

    int nchan;
    CGBitmapInfo ifo;
    switch (pbuf->format) {
    case SG_RGBX: nchan = 4; ifo = kCGImageAlphaNoneSkipLast; break;
    case SG_RGBA: nchan = 4; ifo = kCGImageAlphaLast; break;
    default: assert(0);
    }

    CGColorSpaceRef color_space = CGColorSpaceCreateDeviceRGB();
    assert(color_space != NULL);

    CGDataProviderRef data =  CGDataProviderCreateWithData(
        NULL, pbuf->data, (size_t) pbuf->rowbytes * pbuf->height,
        sg_image_cg_freedata2);
    if (!data)
        abort();

    CGImageRef img = CGImageCreate(
        pbuf->width, pbuf->height,
        8, nchan * 8, pbuf->rowbytes,
        color_space, ifo, data, NULL, false, kCGRenderingIntentDefault);
    if (!img)
        abort();

    CFRelease(color_space);
    CFRelease(data);

    CGDataConsumerCallbacks cb;
    cb.putBytes = sg_pixbuf_cg_iowrite;
    cb.releaseConsumer = sg_pixbuf_cg_iorelease;
    CGDataConsumerRef consumer = CGDataConsumerCreate(fp, &cb);

    CGImageDestinationRef dest = CGImageDestinationCreateWithDataConsumer(
        consumer, kUTTypePNG, 1, NULL);
    if (!dest)
        abort();
    CGImageDestinationAddImage(dest, img, NULL);
    bool success = CGImageDestinationFinalize(dest);
    if (!success)
        abort();

    CFRelease(dest);
    CFRelease(consumer);
    CFRelease(img);

    return 0;
}
