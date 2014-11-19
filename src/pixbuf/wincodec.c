/* Copyright 2012-2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "sg/defs.h"
#include "sg/error.h"
#include "sg/file.h"
#include "sg/log.h"
#include "sg/pixbuf.h"
#include "../core/file_impl.h"
#include "private.h"
#include <string.h>
#include <wincodec.h>

#pragma comment(lib, "WindowsCodecs.lib")

struct sg_image_wic {
    struct sg_image img;
    struct sg_filedata *data;
    IWICImagingFactory *fac;
    IWICBitmapFrameDecode *frame;
};

#define RELEASE(p) do { if ((p) != NULL) (p)->lpVtbl->Release(p); } while (0)

static int
guidcmp(const GUID *x, const GUID *y)
{
    return memcmp(x, y, sizeof(GUID));
}

static void
sg_image_wic_free(struct sg_image *img)
{
    struct sg_image_wic *im = (struct sg_image_wic *) img;
    RELEASE(im->frame);
    RELEASE(im->fac);
    sg_filedata_decref(im->data);
}

static int
sg_image_wic_draw(struct sg_image *img, struct sg_pixbuf *pbuf,
                  int x, int y, struct sg_error **err)
{
    struct sg_image_wic *im = (struct sg_image_wic *) img;
    HRESULT hr;
    IWICFormatConverter *converter = NULL;
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
        (IWICBitmapSource *) im->frame,
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
sg_image_wc(struct sg_filedata *data, struct sg_error **err)
{
    struct sg_image_wic *im = NULL;
    HRESULT hr;
    IWICImagingFactory *fac = NULL;
    IWICStream *stream = NULL;
    IWICBitmapDecoder *decoder = NULL;
    IWICBitmapFrameDecode *frame = NULL;
    WICPixelFormatGUID pixel_format;
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
        (BYTE *) data->data,
        (DWORD) data->length);
    if (hr != S_OK)
        goto failed;

    hr = fac->lpVtbl->CreateDecoderFromStream(
        fac,
        (IStream *) stream,
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

    hr = frame->lpVtbl->GetPixelFormat(frame, &pixel_format);
    if (hr != S_OK)
        goto failed;

    hr = fac->lpVtbl->CreateComponentInfo(fac, &pixel_format, &cinfo);
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
        if (!guidcmp(&pixel_format, &GUID_WICPixelFormat1bppIndexed) ||
            !guidcmp(&pixel_format, &GUID_WICPixelFormat2bppIndexed) ||
            !guidcmp(&pixel_format, &GUID_WICPixelFormat4bppIndexed) ||
            !guidcmp(&pixel_format, &GUID_WICPixelFormat8bppIndexed))
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
        sg_logf(SG_LOG_ERROR, "Unsupported channel count: %u", chanCount);
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
    im->data = data;
    im->fac = fac;
    im->frame = frame;
    sg_filedata_incref(data);
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
sg_image_png(struct sg_filedata *data, struct sg_error **err)
{
    return sg_image_wc(data, err);
}

#endif

#if defined ENABLE_JPEG

struct sg_image *
sg_image_jpeg(struct sg_filedata *data, struct sg_error **err)
{
    return sg_image_wc(data, err);
}

#endif

int
sg_pixbuf_writepng(struct sg_pixbuf *pbuf, const char *path, size_t pathlen,
                   struct sg_error **err)
{
    HRESULT hr;
    IWICImagingFactory *fac = NULL;
    IWICStream *stream = NULL;
    IWICBitmapEncoder *encoder = NULL;
    IWICBitmapFrameEncode *frame = NULL;
    IWICBitmap *bitmap = NULL;
    IWICFormatConverter *converter = NULL;
    WICRect rect;
    wchar_t *fullpath;
    GUID format;
    int r;

    switch (pbuf->format) {
    case SG_RGBX:
        memcpy(&format, &GUID_WICPixelFormat24bppBGR, sizeof(GUID));
        break;
    case SG_RGBA:
        memcpy(&format, &GUID_WICPixelFormat32bppBGRA, sizeof(GUID));
        break;
    default:
        sg_error_invalid(err, __FUNCTION__, "pbuf");
        return -1;
    }

    fullpath = sg_file_createpath(path, pathlen, err);
    if (!fullpath)
        return -1;

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

    hr = stream->lpVtbl->InitializeFromFilename(
        stream,
        fullpath,
        GENERIC_WRITE);
    if (hr != S_OK)
        goto failed;

    hr = fac->lpVtbl->CreateEncoder(
        fac,
        &GUID_ContainerFormatPng,
        NULL,
        &encoder);
    if (hr != S_OK)
        goto failed;

    hr = encoder->lpVtbl->Initialize(
        encoder,
        (IStream *) stream,
        WICBitmapEncoderNoCache);
    if (hr != S_OK)
        goto failed;

    hr = encoder->lpVtbl->CreateNewFrame(
        encoder,
        &frame,
        NULL);
    if (hr != S_OK)
        goto failed;

    hr = frame->lpVtbl->Initialize(frame, NULL);
    if (hr != S_OK)
        goto failed;

    hr = frame->lpVtbl->SetPixelFormat(frame, &format);
    if (hr != S_OK)
        goto failed;

    hr = frame->lpVtbl->SetSize(frame, pbuf->width, pbuf->height);
    if (hr != S_OK)
        goto failed;

    hr = fac->lpVtbl->CreateBitmapFromMemory(
        fac,
        pbuf->width,
        pbuf->height,
        &GUID_WICPixelFormat32bppRGBA,
        pbuf->rowbytes,
        pbuf->height * pbuf->rowbytes,
        pbuf->data,
        &bitmap);
    if (hr != S_OK)
        goto failed;

    hr = fac->lpVtbl->CreateFormatConverter(fac, &converter);
    if (hr != S_OK)
        goto failed;

    hr = converter->lpVtbl->Initialize(
        converter,
        (IWICBitmapSource *) bitmap,
        &format,
        WICBitmapDitherTypeNone,
        NULL,
        0,
        WICBitmapPaletteTypeCustom);
    if (hr != S_OK)
        goto failed;

    rect.X = 0;
    rect.Y = 0;
    rect.Width = pbuf->width;
    rect.Height = pbuf->height;
    hr = frame->lpVtbl->WriteSource(
        frame,
        (IWICBitmapSource *) converter,
        &rect);
    if (hr != S_OK)
        goto failed;

    hr = frame->lpVtbl->Commit(frame);
    if (hr != S_OK)
        goto failed;

    hr = encoder->lpVtbl->Commit(encoder);
    if (hr != S_OK)
        goto failed;

    r = 0;
    goto done;

done:
    RELEASE(converter);
    RELEASE(bitmap);
    RELEASE(frame);
    RELEASE(encoder);
    RELEASE(stream);
    RELEASE(fac);
    free(fullpath);
    return r;

failed:
    sg_error_hresult(err, hr);
    r = -1;
    goto done;
}
