/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "sg/file.h"
#include "sg/pixbuf.h"
#include "private.h"
#include <ApplicationServices/ApplicationServices.h>
#include <assert.h>

static void releaseData(void *info, const void *data, size_t size)
{
    (void) info;
    (void) data;
    (void) size;
}

static CGDataProviderRef
getDataProvider(const void *data, size_t length)
{
    return CGDataProviderCreateWithData(NULL, data, length, releaseData);
}

static int
imageToPixbuf(struct sg_pixbuf *pbuf, CGImageRef img, struct sg_error **err)
{
    // FIXME: We assume here that the image is color.
    size_t w = CGImageGetWidth(img), h = CGImageGetHeight(img), pw, ph;
    CGImageAlphaInfo alphaInfo = CGImageGetAlphaInfo(img);
    sg_pixbuf_format_t pfmt;
    int r, alpha;

    /* FIXME: error here.  */
    assert(w <= INT_MAX && h <= INT_MAX);

    switch (alphaInfo) {
    case kCGImageAlphaNone:
    case kCGImageAlphaNoneSkipLast:
    case kCGImageAlphaNoneSkipFirst:
        alpha = 0;
        break;

    case kCGImageAlphaPremultipliedLast:
    case kCGImageAlphaPremultipliedFirst:
    case kCGImageAlphaLast:
    case kCGImageAlphaFirst:
    default:
        alpha = 1;
        break;
    }
    pfmt = alpha ? SG_RGBA : SG_RGBX;

    r = sg_pixbuf_set(pbuf, pfmt, (int) w, (int) h, err);
    if (r)
        goto error;
    pw = pbuf->pwidth;
    ph = pbuf->pheight;
    r = sg_pixbuf_alloc(pbuf, err);
    if (r)
        goto error;
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    assert(colorSpace);
    CGBitmapInfo info = alpha
        ? kCGImageAlphaPremultipliedLast : kCGImageAlphaNoneSkipLast;
    CGContextRef cxt = CGBitmapContextCreate(
        pbuf->data, pw, ph, 8, pbuf->rowbytes, colorSpace, info);
    assert(cxt);
    CGColorSpaceRelease(colorSpace);

    CGContextSetBlendMode(cxt, kCGBlendModeCopy);
    CGContextSetRGBFillColor(cxt, 0.0f, 0.0f, 0.0f, 0.0f);
    if (w < pw)
        CGContextFillRect(cxt, CGRectMake(w, 0, pw - w, h));
    if (h < ph)
        CGContextFillRect(cxt, CGRectMake(0, 0, pw, pw - h));
    CGContextDrawImage(cxt, CGRectMake(0, ph - h, w, h), img);

    CGImageRelease(img);
    CGContextRelease(cxt);
    return 0;

error:
    return -1;
}

int
sg_pixbuf_loadpng(struct sg_pixbuf *pbuf, const void *data, size_t length,
                  struct sg_error **err)
{
    CGDataProviderRef dp = getDataProvider(data, length);
    assert(dp);
    CGImageRef img = CGImageCreateWithPNGDataProvider(
        dp, NULL, false, kCGRenderingIntentDefault);
    CGDataProviderRelease(dp);
    assert(img);
    return imageToPixbuf(pbuf, img, err);
}

int
sg_pixbuf_loadjpeg(struct sg_pixbuf *pbuf, const void *data, size_t length,
                   struct sg_error **err)
{
    CGDataProviderRef dp = getDataProvider(data, length);
    assert(dp);
    CGImageRef img = CGImageCreateWithJPEGDataProvider(
        dp, NULL, false, kCGRenderingIntentDefault);
    CGDataProviderRelease(dp);
    assert(img);
    return imageToPixbuf(pbuf, img, err);
}

static size_t
sgFilePutBytes(void *info, const void *buffer, size_t count)
{
    struct sg_file *fp = info;
    int r = fp->write(fp, buffer, count);
    return r < 0 ? 0 : (size_t) r;
}

static void
sgFileRelease(void *info)
{
    (void) info;
}

static CGDataConsumerRef
getDataConsumer(struct sg_file *fp)
{
    CGDataConsumerCallbacks cb;
    cb.putBytes = sgFilePutBytes;
    cb.releaseConsumer = sgFileRelease;
    CGDataConsumerRef dc = CGDataConsumerCreate(fp, &cb);
    assert(dc != NULL);
    return dc;
}

int
sg_pixbuf_writepng(struct sg_pixbuf *pbuf, struct sg_file *fp,
                   struct sg_error **err)
{
    int nchan;
    CGBitmapInfo ifo;
    switch (pbuf->format) {
        case SG_Y:    nchan = 1; ifo = kCGImageAlphaNone; break;
        case SG_YA:   nchan = 2; ifo = kCGImageAlphaLast; break;
        /* SG_RGB not supported */
        case SG_RGBA: nchan = 4; ifo = kCGImageAlphaNoneSkipLast; break;
        /* case SG_RGBA: nchan = 4; ifo = kCGImageAlphaPremultipliedLast;
           break; */
        default: assert(0);
    }

    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    assert(colorSpace != NULL);

    CGDataProviderRef dp = getDataProvider(
        pbuf->data, (size_t) pbuf->rowbytes * pbuf->iheight);
    assert(dp != NULL);

    CGImageRef img = CGImageCreate(
        pbuf->iwidth, pbuf->iheight,
        8, nchan * 8, pbuf->rowbytes,
        colorSpace, ifo, dp, NULL, false, kCGRenderingIntentDefault);
    assert(img != NULL);

    CGDataConsumerRef dc = getDataConsumer(fp);

    CGImageDestinationRef dest = CGImageDestinationCreateWithDataConsumer(
        dc, kUTTypePNG, 1, NULL);
    assert(dest != NULL);

    CGImageDestinationAddImage(dest, img, NULL);
    bool br = CGImageDestinationFinalize(dest);
    assert(br);

    CFRelease(dest);
    CFRelease(dc);
    CFRelease(img);
    CFRelease(dp);
    CFRelease(colorSpace);

    return 0;
    (void) err;
}
