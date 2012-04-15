#include "pixbuf.h"
#include <ApplicationServices/ApplicationServices.h>

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
    // pfmt = alpha ? SG_RGBA : SG_RGB;
    pfmt = SG_RGBA; // FIXME

    r = sg_pixbuf_set(pbuf, pfmt, w, h, err);
    if (r)
        goto error;
    pw = pbuf->pwidth;
    ph = pbuf->pheight;
    r = sg_pixbuf_alloc(pbuf, err);
    if (r)
        goto error;
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    assert(colorSpace);
    CGBitmapInfo info = (alpha ? kCGImageAlphaPremultipliedLast : kCGImageAlphaNoneSkipLast);
    CGContextRef cxt = CGBitmapContextCreate(pbuf->data, pw, ph, 8, pbuf->rowbytes, colorSpace, info);
    assert(cxt);
    CGColorSpaceRelease(colorSpace);

    CGContextSetBlendMode(cxt, kCGBlendModeCopy);
    CGContextSetRGBFillColor(cxt, 0.0f, 0.0f, 0.0f, 0.0f);
    if (w < pw)
        CGContextFillRect(cxt, CGRectMake(w, 0, pw - w, h));
    if (h < ph)
        CGContextFillRect(cxt, CGRectMake(0, h, pw, pw - h));
    CGContextDrawImage(cxt, CGRectMake(0, 0, w, h), img);

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
    CGImageRef img = CGImageCreateWithPNGDataProvider(dp, NULL, false, kCGRenderingIntentDefault);
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
    CGImageRef img = CGImageCreateWithJPEGDataProvider(dp, NULL, false, kCGRenderingIntentDefault);
    CGDataProviderRelease(dp);
    assert(img);
    return imageToPixbuf(pbuf, img, err);
}
