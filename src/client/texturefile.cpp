#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if defined(_WIN32)
// Known Visual Studio bug: 621653
#pragma warning (disable : 4005)
#endif

#include "texturefile.hpp"
#include "sys/file.hpp"
// #include "sys/path.hpp"
#include <stdlib.h>
#include <stdio.h>
// #include <err.h>
#include <set>
#include <memory>
#include <vector>
#include "opengl.hpp"

static const unsigned MAX_SIZE = 2048;

static const std::string filePrefix("file:");

struct TextureFileCompare {
    bool operator()(TextureFile *x, TextureFile *y)
    { return x->path() < y->path(); }
};

typedef std::set<TextureFile *, TextureFileCompare> TextureFileSet;
static TextureFileSet textureFiles;

TextureFile::Ref TextureFile::open(std::string const &path)
{
    TextureFile *tp;
    std::auto_ptr<TextureFile> t(new TextureFile(path));
    std::pair<TextureFileSet::iterator, bool> r =
        textureFiles.insert(t.get());
    if (r.second) {
        tp = t.release();
        tp->registerTexture();
    } else
        tp = *r.first;
    return Ref(tp);
}

TextureFile::TextureFile(std::string const &path)
    : path_(path)
{ }

TextureFile::~TextureFile()
{
    TextureFileSet::iterator i = textureFiles.find(this);
    if (i != textureFiles.end() && *i == this)
        textureFiles.erase(i);
}

std::string TextureFile::name() const
{
    return filePrefix + path_;
}

bool TextureFile::loadTexture()
{
    std::string::const_iterator
        b = path_.begin(), p = path_.end(), d = p, e = p;
    while (p != b) {
        --p;
        if (*p == '.') {
            d = p + 1;
            break;
        }
    }
    std::string suffix(d, e);
    if (suffix == "png")
        return loadPNG();
    else if (suffix == "jpg" || suffix == "jpeg")
        return loadJPEG();
    return false;
}


#if defined(HAVE_COREGRAPHICS)
#include <CoreGraphics/CoreGraphics.h>

static void releaseData(void *info, const void *data, size_t size)
{
    (void) data;
    (void) size;
    free(info);
}

static CGDataProviderRef getDataProvider(const std::string &path)
{
    std::auto_ptr<IFile> f(Path::openIFile(path));
    Buffer buf = f->readall();
    CGDataProviderRef dp = CGDataProviderCreateWithData(buf.get(), buf.get(), buf.size(), releaseData);
    assert(dp);
    buf.release();
    return dp;
}

static bool imageToTexture(TextureFile &tex, CGImageRef img)
{
    // FIXME: We assume here that the image is color.
    size_t w = CGImageGetWidth(img), h = CGImageGetHeight(img);
    CGImageAlphaInfo alphaInfo = CGImageGetAlphaInfo(img);
    bool hasAlpha;

    switch (alphaInfo) {
    case kCGImageAlphaNone:
    case kCGImageAlphaNoneSkipLast:
    case kCGImageAlphaNoneSkipFirst:
        hasAlpha = false;
        break;

    case kCGImageAlphaPremultipliedLast:
    case kCGImageAlphaPremultipliedFirst:
    case kCGImageAlphaLast:
    case kCGImageAlphaFirst:
    default:
        hasAlpha = true;
        break;
    }

    tex.alloc(w, h, true, true);
    size_t tw = tex.twidth(), th = tex.theight(); 
    printf("tw = %zu, th = %zu\n", tw, th);
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    assert(colorSpace);
    CGBitmapInfo info = (hasAlpha ? kCGImageAlphaPremultipliedLast : kCGImageAlphaNoneSkipLast);
    CGContextRef cxt = CGBitmapContextCreate(tex.buf(), tw, th, 8, tex.rowbytes(), colorSpace, info);
    assert(cxt);
    CGColorSpaceRelease(colorSpace);

    CGContextSetRGBFillColor(cxt, 1.0f, 0.0f, 0.0f, 0.0f);
    if (w < tw)
        CGContextFillRect(cxt, CGRectMake(w, 0, tw - w, h));
    if (h < th)
        CGContextFillRect(cxt, CGRectMake(0, h, tw, tw - h));
    CGContextDrawImage(cxt, CGRectMake(0, 0, w, h), img);

    CGImageRelease(img);
    CGContextRelease(cxt);
    return true;
}

bool TextureFile::loadPNG()
{
    CGDataProviderRef dp = getDataProvider(path_);
    assert(dp);
    CGImageRef img = CGImageCreateWithPNGDataProvider(dp, NULL, false, kCGRenderingIntentDefault);
    CGDataProviderRelease(dp);
    assert(img);
    return imageToTexture(*this, img);
}

bool TextureFile::loadJPEG()
{
    CGDataProviderRef dp = getDataProvider(path_);
    assert(dp);
    CGImageRef img = CGImageCreateWithJPEGDataProvider(dp, NULL, false, kCGRenderingIntentDefault);
    CGDataProviderRelease(dp);
    assert(img);
    return imageToTexture(*this, img);
}

#elif defined(_WIN32)
#include <wincodec.h>

#pragma comment(lib, "WindowsCodecs.lib")

bool TextureFile::loadPNG()
{
    std::auto_ptr<IFile> f(Path::openIFile(path_));
    Buffer buffer = f->readall();
    f.reset();

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
    bool color, alpha, success = false;
    unsigned tw, th, tstride;

    hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&fac));
    if (hr != S_OK)
        goto failed;
    hr = fac->CreateStream(&stream);
    if (hr != S_OK)
        goto failed;
    hr = stream->InitializeFromMemory(buffer.getUC(), buffer.size());
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
        color = false;
        alpha = false;
        targetFormat = GUID_WICPixelFormat8bppGray;
        break;

    case 3:
        color = true;
        alpha = false;
        targetFormat = GUID_WICPixelFormat24bppRGB;
        break;

    case 4:
        color = true;
        alpha = true;
        targetFormat = GUID_WICPixelFormat32bppRGBA;
        break;

    default:
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
    
    alloc(iwidth, iheight, color, alpha);
    tw = twidth();
    th = theight(); 
    tstride = rowbytes();
    converter->CopyPixels(NULL, tstride, th * tstride, (BYTE *) buf());
    success = true;

failed:
    if (converter) converter->Release();
    if (pinfo) pinfo->Release();
    if (cinfo) cinfo->Release();
    if (frame) frame->Release();
    if (decoder) decoder->Release();
    if (stream) stream->Release();
    if (fac) fac->Release();
    return success;
}

bool TextureFile::loadJPEG()
{
    return loadPNG();
}

#else /* !HAVE_COREGRAPHICS !_WIN32 */

#if defined(HAVE_LIBPNG)
#include <png.h>

struct PNGStruct {
    PNGStruct() : png(0), info(0) { }
    ~PNGStruct() { png_destroy_read_struct(&png, &info, 0); }
    png_structp png;
    png_infop info;
};

static void textureFileRead(png_structp pngp,
                            png_bytep data, png_size_t length) throw ()
{
    try {
        File &f = *reinterpret_cast<File *> (png_get_io_ptr(pngp));
        size_t pos = 0;
        while (pos < length) {
            size_t amt = f.read(data + pos, length - pos);
            if (amt == 0) {
                fputs("Unexpected end of file\n", stderr);
                longjmp(png_jmpbuf(pngp), 1);
            }
            pos += amt;
        }
    } catch (std::exception const &e) {
        fprintf(stderr, "Error: %s\n", e.what());
        longjmp(png_jmpbuf(pngp), 1);
    }
}

// FIXME error handling
bool TextureFile::loadPNG()
{
    PNGStruct ps;
    int depth, ctype;
    png_uint_32 width, height;
    uint32_t th, i, rowbytes, chan;
    bool color = false, alpha = false;
    unsigned char *data = NULL;
    std::vector<png_bytep> rows;

    File f(path_.c_str(), 0);

    ps.png = png_create_read_struct_2(
        PNG_LIBPNG_VER_STRING,
        NULL, NULL, NULL, NULL, NULL, NULL);
    if (!ps.png) goto pngerr;
    ps.info = png_create_info_struct(ps.png);
    if (!ps.info) {
    pngerr:
        fputs("Failed to initialize LibPNG\n", stderr);
        return false;
    }

    if (setjmp(png_jmpbuf(ps.png)))
        return false;

    png_set_read_fn(ps.png, &f, textureFileRead);

    png_read_info(ps.png, ps.info);
    png_get_IHDR(ps.png, ps.info, &width, &height,
                 &depth, &ctype, NULL, NULL, NULL);

    switch (ctype) {
    case PNG_COLOR_TYPE_PALETTE:
        color = true;
        png_set_palette_to_rgb(ps.png);
        break;
    case PNG_COLOR_TYPE_GRAY:
        if (depth < 8)
            png_set_expand_gray_1_2_4_to_8(ps.png);
        break;
    case PNG_COLOR_TYPE_GRAY_ALPHA:
        alpha = true;
        break;
    case PNG_COLOR_TYPE_RGB:
        color = true;
        break;
    case PNG_COLOR_TYPE_RGB_ALPHA:
        color = true;
        alpha = true;
        break;
    }
    if (depth > 8)
        png_set_strip_16(ps.png);

    alloc(width, height, color, alpha);
    th = this->theight();
    chan = channels();
    rowbytes = this->rowbytes();
    data = reinterpret_cast<unsigned char *>(buf());
    if (width * chan < rowbytes) {
        for (i = 0; i < height; ++i)
            memset(data + rowbytes * i + width * chan, 0,
                   rowbytes - width * chan);
    } else
        i = height;
    for (; i < th; ++i)
        memset(data + rowbytes * i, 0, rowbytes);
    rows.resize(height);
    for (i = 0; i < height; ++i)
        rows[i] = data + rowbytes * i;
    png_read_image(ps.png, &rows.front());
    png_read_end(ps.png, NULL);
    return true;
}

#else /* !HAVE_LIBPNG */
#error "Can't load PNG images"
#endif

#if defined(HAVE_LIBJPEG)
#include <jpeglib.h>

// These few methods for the input source are
// basically lifted from the LibJPEG source code,
// for a *later version* with a "memory source".

static void jInitSource(j_decompress_ptr cinfo)
{
    (void) cinfo;
}

static void jTermSource(j_decompress_ptr cinfo)
{
    (void) cinfo;
}

static boolean jFillInputBuffer(j_decompress_ptr cinfo)
{
    static JOCTET buf[4];
    // WARN
    buf[0] = 0xff;
    buf[1] = JPEG_EOI;
    cinfo->src->next_input_byte = buf;
    cinfo->src->bytes_in_buffer = 2;
    return TRUE;
}

static void jSkipInputData(j_decompress_ptr cinfo, long num_bytes)
{
    struct jpeg_source_mgr *src = cinfo->src;
    if (num_bytes > 0) {
        while (num_bytes > (long) src->bytes_in_buffer) {
            num_bytes -= (long) src->bytes_in_buffer;
            (void) (*src->fill_input_buffer) (cinfo);
        }
        src->next_input_byte += (size_t) num_bytes;
        src->bytes_in_buffer -= (size_t) num_bytes;
    }
}

struct jStruct {
    jStruct()
    {
        jpeg_create_decompress(&cinfo);
    }

    ~jStruct()
    {
        jpeg_destroy_decompress(&cinfo);
    }

    struct jpeg_decompress_struct cinfo;
};

// FIXME error handling
bool TextureFile::loadJPEG()
{
    jStruct jj;
    struct jpeg_error_mgr jerr;
    struct jpeg_source_mgr jsrc;
    unsigned w, h, rb, i;
    unsigned char *jdata = NULL, *jptr[1];
    bool color;

    FBuffer b(path_.c_str(), 0, -1);
    if (!b.size())
        return false;

    jj.cinfo.err = jpeg_std_error(&jerr);

    jj.cinfo.src = &jsrc;
    jsrc.next_input_byte = b.getUC();
    jsrc.bytes_in_buffer = b.size();
    jsrc.init_source =  jInitSource;
    jsrc.fill_input_buffer = jFillInputBuffer;
    jsrc.skip_input_data = jSkipInputData;
    jsrc.resync_to_restart = jpeg_resync_to_restart;
    jsrc.term_source = jTermSource;

    jpeg_read_header(&jj.cinfo, TRUE);

    w = jj.cinfo.image_width;
    h = jj.cinfo.image_height;
    if (w > MAX_SIZE || h > MAX_SIZE) {
        fprintf(stderr, "%s: too big\n", path_.c_str());
        return false;
    }

    switch (jj.cinfo.out_color_space) {
    case JCS_GRAYSCALE:
        color = false;
        break;

    case JCS_RGB:
        color = true;
        break;

    default:
        fprintf(stderr, "%s: unknown color space\n", path_.c_str());
        return false;
    }

    jpeg_start_decompress(&jj.cinfo);

    alloc(w, h, color, false);
    rb = rowbytes();
    jdata = reinterpret_cast<unsigned char *> (buf());
    while ((unsigned) jj.cinfo.output_scanline < h) {
        for (i = 0; i < 1; ++i)
            jptr[i] = jdata + jj.cinfo.output_scanline * (rb + i);
        jpeg_read_scanlines(&jj.cinfo, jptr, 1);
    }

    jpeg_finish_decompress(&jj.cinfo);
    return true;
}

#else /* !HAVE_LIBJPEG */
#error "Can't load JPEG images"
#endif

#endif
