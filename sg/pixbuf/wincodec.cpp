#include "defs.h"

#include "error.h"
#include "log.h"
#include "pixbuf.h"
#include <wincodec.h>

#pragma comment(lib, "WindowsCodecs.lib")

static int
sg_pixbuf_loadwincodec(struct sg_pixbuf *pbuf, const void *data, size_t len,
                       struct sg_error **err)
{
    HRESULT hr;
    IWICImagingFactory *fac = NULL;
    IWICStream *stream = NULL;
    IWICBitmapDecoder *decoder = NULL;
    IWICBitmapFrameDecode *frame = NULL;
    WICPixelFormatGUID pixelFormat, targetFormat;
    IWICComponentInfo *cinfo = NULL;
    IWICPixelFormatInfo *pinfo = NULL;
    IWICFormatConverter *converter = NULL;
    UINT chanCount, iwidth, iheight;
    sg_pixbuf_format_t pfmt;
    int r;

    hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&fac));
    if (hr != S_OK)
        goto failed;
    hr = fac->CreateStream(&stream);
    if (hr != S_OK)
        goto failed;
    hr = stream->InitializeFromMemory((BYTE *) data, (DWORD) len);
    if (hr != S_OK)
        goto failed;
    hr = fac->CreateDecoderFromStream(stream, NULL, WICDecodeMetadataCacheOnDemand, &decoder);
    if (hr != S_OK)
        goto failed;
    hr = decoder->GetFrame(0, &frame);
    if (hr != S_OK)
        goto failed;
    hr = frame->GetPixelFormat(&pixelFormat);
    if (hr != S_OK)
        goto failed;
    hr = fac->CreateComponentInfo(pixelFormat, &cinfo);
    if (hr != S_OK)
        goto failed;
    hr = cinfo->QueryInterface(IID_IWICPixelFormatInfo, (void **) &pinfo);
    if (hr != S_OK)
        goto failed;
    hr = pinfo->GetChannelCount(&chanCount);
    if (hr != S_OK)
        goto failed;

    switch (chanCount) {
    case 1:
        if (pixelFormat == GUID_WICPixelFormat1bppIndexed ||
            pixelFormat == GUID_WICPixelFormat2bppIndexed ||
            pixelFormat == GUID_WICPixelFormat4bppIndexed ||
            pixelFormat == GUID_WICPixelFormat8bppIndexed)
        {
            pfmt = SG_RGBA;
            targetFormat = GUID_WICPixelFormat32bppPRGBA;
        } else {
            pfmt = SG_Y;
            targetFormat = GUID_WICPixelFormat8bppGray;
        }
        break;

    case 3:
    case 4:
        pfmt = SG_RGBA;
        targetFormat = GUID_WICPixelFormat32bppPRGBA;
        break;

    default:
        sg_logf(sg_logger_get("image"), LOG_ERROR,
                "Unsupported channel count: %u", chanCount);
        sg_error_data(err, "Wincodec");
        goto failed;
    }

    hr = fac->CreateFormatConverter(&converter);
    if (hr != S_OK)
        goto failed;
    hr = converter->Initialize(frame, targetFormat, WICBitmapDitherTypeNone, NULL, 0, WICBitmapPaletteTypeCustom);
    if (hr != S_OK)
        goto failed;
    hr = converter->GetSize(&iwidth, &iheight);
    if (hr != S_OK)
        goto failed;
    
    r = sg_pixbuf_set(pbuf, pfmt, iwidth, iheight, err);
    if (r)
        goto done;
    r = sg_pixbuf_alloc(pbuf, err);
    if (r)
        goto done;
    converter->CopyPixels(NULL, pbuf->rowbytes, pbuf->rowbytes * pbuf->pheight, (BYTE *) pbuf->data);
    r = 0;

done:
    if (converter) converter->Release();
    if (pinfo) pinfo->Release();
    if (cinfo) cinfo->Release();
    if (frame) frame->Release();
    if (decoder) decoder->Release();
    if (stream) stream->Release();
    if (fac) fac->Release();
    return r;

failed:
    sg_error_hresult(err, hr);
    r = -1;
    goto done;
}

int
sg_pixbuf_loadpng(struct sg_pixbuf *pbuf, const void *data, size_t len,
                  struct sg_error **err)
{
    return sg_pixbuf_loadwincodec(pbuf, data, len, err);
}

int
sg_pixbuf_loadjpeg(struct sg_pixbuf *pbuf, const void *data, size_t len,
                   struct sg_error **err)
{
    return sg_pixbuf_loadwincodec(pbuf, data, len, err);
}
