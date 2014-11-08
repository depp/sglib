/* Copyright 2012-2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "sg/defs.h"
#include "sg/error.h"
#include "sg/file.h"
#include "sg/log.h"
#include "sg/pixbuf.h"
#include "private.h"
#include <string.h>
#include <wincodec.h>

#pragma comment(lib, "WindowsCodecs.lib")

struct sg_image_wic {
    struct sg_image img;
    struct sg_buffer *buf;
    IWICImagingFactory *fac;
    IWICBitmapFrameDecode *frame;
};

#define RELEASE(p) do { if ((p) != NULL) (p)->lpVtbl->Release(p); } while (0)

static int
cmpguid(const GUID *x, const GUID *y)
{
    return memcmp(x, y, sizeof(GUID));
}

static void
sg_image_wic_free(struct sg_image *img)
{
    struct sg_image_wic *im = (struct sg_image_wic *) img;
    RELEASE(im->frame);
    RELEASE(im->fac);
    sg_buffer_decref(im->buf);
}

static int
sg_image_wic_draw(struct sg_image *img, struct sg_pixbuf *pbuf,
                  int x, int y, struct sg_error **err)
{
    struct sg_image_wic *im = (struct sg_image_wic *) img;
    HRESULT hr;
    IWICFormatConverter *converter = NULL;
    WICPixelFormatGUID target_format;
    int r = 0;

    if (pbuf->format != SG_RGBX && pbuf->format != SG_RGBA) {
        sg_error_invalid(err, __FUNCTION__, "pbuf");
        return -1;
    }

    hr = im->fac->lpVtbl->CreateFormatConverter(im->fac, &converter);
    if (hr != S_OK)
        goto failed;

    hr = converter->lpVtbl->Initialize(
        converter,
        im->frame,
        &GUID_WICPixelFormat32bppPRGBA,
        WICBitmapDitherTypeNone,
        NULL,
        0,
        WICBitmapPaletteTypeCustom);
    if (hr != S_OK)
        goto failed;

    hr = converter->lpVtbl->CopyPixels(
        converter,
        NULL,
        pbuf->rowbytes,
        pbuf->rowbytes * pbuf->height,
        (BYTE *) pbuf->data + (pbuf->rowbytes) * y + x * 4);
    if (hr != S_OK)
        goto failed;

    r = 0;
    goto done;

done:
    RELEASE(converter);
    return r;

failed:
    sg_error_hresult(err, hr);
    r = -1;
    goto done;
}

struct sg_image *
sg_image_wc(struct sg_buffer *buf, struct sg_error **err)
{
    struct sg_image_wic *im = NULL;
    HRESULT hr;
    IWICImagingFactory *fac = NULL;
    IWICStream *stream = NULL;
    IWICBitmapDecoder *decoder = NULL;
    IWICBitmapFrameDecode *frame = NULL;
    WICPixelFormatGUID pixelFormat, targetFormat;
    IWICComponentInfo *cinfo = NULL;
    IWICPixelFormatInfo2 *pinfo = NULL;
    UINT chanCount, width, height;
    unsigned flags;

    hr = CoCreateInstance(
        &CLSID_WICImagingFactory,
        NULL,
        CLSCTX_INPROC_SERVER,
        &IID_IWICImagingFactory,
        (void **) &fac);
    if (hr != S_OK)
        goto failed;

    hr = fac->lpVtbl->CreateStream(fac, &stream);
    if (hr != S_OK)
        goto failed;

    hr = stream->lpVtbl->InitializeFromMemory(
        stream,
        (BYTE *) buf->data,
        (DWORD) buf->length);
    if (hr != S_OK)
        goto failed;

    hr = fac->lpVtbl->CreateDecoderFromStream(
        fac,
        stream,
        NULL,
        WICDecodeMetadataCacheOnDemand,
        &decoder);
    if (hr != S_OK)
        goto failed;

    hr = decoder->lpVtbl->GetFrame(decoder, 0, &frame);
    if (hr != S_OK)
        goto failed;

    hr = frame->lpVtbl->GetSize(frame, &width, &height);
    if (hr != S_OK)
        goto failed;

    hr = frame->lpVtbl->GetPixelFormat(frame, &pixelFormat);
    if (hr != S_OK)
        goto failed;

    hr = fac->lpVtbl->CreateComponentInfo(fac, &pixelFormat, &cinfo);
    if (hr != S_OK)
        goto failed;

    hr = cinfo->lpVtbl->QueryInterface(
        cinfo,
        &IID_IWICPixelFormatInfo2,
        (void **) &pinfo);
    if (hr != S_OK)
        goto failed;

    hr = pinfo->lpVtbl->GetChannelCount(pinfo, &chanCount);
    if (hr != S_OK)
        goto failed;

    switch (chanCount) {
    case 1:
        if (!cmpguid(&pixelFormat, &GUID_WICPixelFormat1bppIndexed) ||
            !cmpguid(&pixelFormat, &GUID_WICPixelFormat2bppIndexed) ||
            !cmpguid(&pixelFormat, &GUID_WICPixelFormat4bppIndexed) ||
            !cmpguid(&pixelFormat, &GUID_WICPixelFormat8bppIndexed))
        {
            /* This is not so precise, but we don't care */
            flags = SG_IMAGE_COLOR | SG_IMAGE_ALPHA;
        } else {
            flags = 0;
        }
        break;

    case 3:
        flags = SG_IMAGE_COLOR;
        break;

    case 4:
        flags = SG_IMAGE_COLOR | SG_IMAGE_ALPHA;
        break;

    default:
        sg_logf(sg_logger_get("image"), SG_LOG_ERROR,
                "Unsupported channel count: %u", chanCount);
        sg_error_data(err, "Wincodec");
        goto failed;
    }

    im = malloc(sizeof(*im));
    if (!im) {
        sg_error_nomem(err);
        goto done;
    }
    im->img.width = width;
    im->img.height = height;
    im->img.flags = flags;
    im->img.free = sg_image_wic_free;
    im->img.draw = sg_image_wic_draw;
    im->buf = buf;
    im->fac = fac;
    im->frame = frame;
    sg_buffer_incref(buf);
    fac = NULL;
    frame = NULL;
    goto done;

done:
    RELEASE(pinfo);
    RELEASE(cinfo);
    RELEASE(frame);
    RELEASE(decoder);
    RELEASE(stream);
    RELEASE(fac);
    if (im != NULL)
        return &im->img;
    return NULL;

failed:
    sg_error_hresult(err, hr);
    goto done;
}

#if defined ENABLE_PNG

struct sg_image *
sg_image_png(struct sg_buffer *buf, struct sg_error **err)
{
    return sg_image_wc(buf, err);
}

#endif

#if defined ENABLE_JPEG

struct sg_image *
sg_image_jpeg(struct sg_buffer *buf, struct sg_error **err)
{
    return sg_image_wc(buf, err);
}

#endif

int
sg_pixbuf_writepng(struct sg_pixbuf *pbuf, struct sg_file *fp,
                   struct sg_error **err)
{
    abort();
    return -1;
}
