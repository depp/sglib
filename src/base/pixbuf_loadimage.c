#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define IMAGE_MAXSIZE (16 * 1024 * 1024)

#include "error.h"
#include "file.h"
#include "pixbuf.h"
#include <stdlib.h>
#include <string.h>

#if defined(HAVE_COREGRAPHICS)
#include <CoreGraphics/CoreGraphics.h>

static void releaseData(void *info, const void *data, size_t size)
{
    (void) info;
    (void) data;
    (void) size;
}

static CGDataProviderRef
getDataProvider(struct sg_buffer *fbuf)
{
    return CGDataProviderCreateWithData(fbuf->data, fbuf->data, fbuf->length, releaseData);
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

    CGContextSetRGBFillColor(cxt, 1.0f, 0.0f, 0.0f, 0.0f);
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

static int
sg_pixbuf_loadpng(struct sg_pixbuf *pbuf, struct sg_buffer *fbuf,
                  struct sg_error **err)
{
    CGDataProviderRef dp = getDataProvider(fbuf);
    assert(dp);
    CGImageRef img = CGImageCreateWithPNGDataProvider(dp, NULL, false, kCGRenderingIntentDefault);
    CGDataProviderRelease(dp);
    assert(img);
    return imageToPixbuf(pbuf, img, err);
}

static int
sg_pixbuf_loadjpeg(struct sg_pixbuf *pbuf, struct sg_buffer *fbuf,
                   struct sg_error **err)
{
    CGDataProviderRef dp = getDataProvider(fbuf);
    assert(dp);
    CGImageRef img = CGImageCreateWithJPEGDataProvider(dp, NULL, false, kCGRenderingIntentDefault);
    CGDataProviderRelease(dp);
    assert(img);
    return imageToPixbuf(pbuf, img, err);
}

#else

#if defined(HAVE_LIBPNG)
#include <png.h>

static void
sg_png_message(png_struct *pngp, int iserror, const char *msg)
{
    (void) pngp;
    (void) iserror;
    fprintf(stderr, "libpng: %s", msg);
}

static void
sg_png_error(png_struct *pngp, const char *msg)
{
    struct sg_error **err = png_get_error_ptr(pngp);
    sg_png_message(pngp, 1, msg);
    sg_error_data(err, "PNG");
    longjmp(png_jmpbuf(pngp), 1);
}

static void
sg_png_warning(png_struct *pngp, const char *msg)
{
    sg_png_message(pngp, 0, msg);
}

struct sg_pngbuf {
    const char *p, *e;
};

static void
sg_png_read(png_struct *pngp, unsigned char *ptr, png_size_t len)
{
    struct sg_pngbuf *b;
    size_t rem;
    b = png_get_io_ptr(pngp);
    rem = b->e - b->p;
    if (len <= rem) {
        memcpy(ptr, b->p, len);
        b->p += len;
    } else {
        sg_png_error(pngp, "unexpected end of file");
    }
}

static int
sg_pixbuf_loadpng(struct sg_pixbuf *pbuf, struct sg_buffer *fbuf,
                  struct sg_error **err)
{
    png_struct *volatile pngp = NULL;
    png_info *volatile infop = NULL;
    struct sg_pngbuf rbuf;
    png_uint_32 width, height;
    unsigned i;
    int r, depth, ctype, ret;
    sg_pixbuf_format_t pfmt;
    unsigned char **rowp;
    void *volatile tmp = NULL;

    rbuf.p = fbuf->data;
    rbuf.e = rbuf.p + fbuf->length;

    pngp = png_create_read_struct_2(
        PNG_LIBPNG_VER_STRING,
        err, sg_png_error, sg_png_warning, NULL, NULL, NULL);
    if (!pngp)
        goto error;
    infop = png_create_info_struct(pngp);
    if (!infop)
        goto error;

    if (setjmp(png_jmpbuf(pngp)))
        goto error;

    png_set_read_fn(pngp, &rbuf, sg_png_read);

    png_read_info(pngp, infop);
    png_get_IHDR(pngp, infop, &width, &height, &depth, &ctype,
                 NULL, NULL, NULL);

    switch (ctype) {
    case PNG_COLOR_TYPE_PALETTE:
        pfmt = SG_RGB;
        png_set_palette_to_rgb(pngp);
        break;

    case PNG_COLOR_TYPE_GRAY:
        pfmt = SG_Y;
        if (depth < 8)
            png_set_expand_gray_1_2_4_to_8(pngp);
        break;

    case PNG_COLOR_TYPE_GRAY_ALPHA:
        pfmt = SG_YA;
        break;

    case PNG_COLOR_TYPE_RGB:
        pfmt = SG_RGB;
        break;

    case PNG_COLOR_TYPE_RGB_ALPHA:
        pfmt = SG_RGBA;
        break;

    default:
        goto error;
    }

    if (depth > 8)
        png_set_strip_16(pngp);

    r = sg_pixbuf_set(pbuf, pfmt, width, height, err);
    if (r)
        goto error;
    r = sg_pixbuf_alloc(pbuf, err);
    if (r)
        goto error;

    rowp = malloc(height * pbuf->rowbytes);
    if (!rowp)
        goto error;
    tmp = rowp;
    for (i = 0; i < height; ++i)
        rowp[i] = (unsigned char *) pbuf->data + pbuf->rowbytes * i;
    png_read_image(pngp, rowp);
    png_read_end(pngp, NULL);

    ret = 0;
    goto done;

error:
    ret = -1;
    goto done;

done:
    if (tmp)
        free(tmp);
    png_destroy_read_struct((png_structp *) &pngp,
                            (png_infop *) &infop, NULL);
    return ret;
}

#else

#error "Can't load PNG images."

#endif

#if defined(HAVE_LIBJPEG)
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

static int
sg_pixbuf_loadjpeg(struct sg_pixbuf *pbuf, struct sg_buffer *fbuf,
                   struct sg_error **err)
{
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    struct jpeg_source_mgr jsrc;
    unsigned width, height, rowbytes;
    sg_pixbuf_format_t pfmt;
    unsigned char *data, *rowp[1];
    int ret, r;

    jpeg_create_decompress(&cinfo);

    /* FIXME: this aborts.  */
    cinfo.err = jpeg_std_error(&jerr);
    cinfo.src = &jsrc;

    jsrc.next_input_byte = fbuf->data;
    jsrc.bytes_in_buffer = fbuf->length;
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
        fprintf(stderr, "JPEG: unknown color space\n");
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
    data = pbuf->data;
    while (cinfo.output_scanline < height) {
        rowp[0] = data + cinfo.output_scanline * rowbytes;
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

#else

#error "Can't load JPEG files."

#endif

#endif

enum {
    IMG_PNG,
    IMG_JPEG
};

int
sg_pixbuf_loadimage(struct sg_pixbuf *pbuf, const char *path,
                    struct sg_error **err)
{
    struct sg_buffer fbuf;
    char npath[SG_MAX_PATH + 8];
    int r, c, attempt = -1;
    size_t l, maxsize;
    unsigned e, i;
    struct sg_error *ferr = NULL;

    maxsize = IMAGE_MAXSIZE;
    l = strlen(path);
    if (l >= SG_MAX_PATH)
        return -1;
    for (e = l; e && path[e-1] != '/' && path[e-1] != '.'; --e) { }
    if (!e || path[e-1] == '/') {
        /* The path has no extension.  */
        e = l;
    } else {
        /* The path has an extension.  */
        for (i = e; i < l; ++i) {
            c = path[i];
            if (c >= 'A' && c <= 'Z')
                c += 'a' - 'A';
            npath[i] = c;
        }
        if (l - e == 3) {
            if (!memcmp(npath + e, "png", 3)) {
                attempt = IMG_PNG;
                r = sg_file_get(path, 0, &fbuf, maxsize, &ferr);
                if (!r) {
                    r = sg_pixbuf_loadpng(pbuf, &fbuf, err);
                    goto havefile;
                }
            } else if (!memcmp(npath + e, "jpg", 3)) {
                attempt = IMG_JPEG;
                r = sg_file_get(path, 0, &fbuf, maxsize, &ferr);
                if (!r) {
                    r = sg_pixbuf_loadjpeg(pbuf, &fbuf, err);
                    goto havefile;
                }
            }
        }
        if (attempt >= 0) {
            if (ferr->domain != &SG_ERROR_NOTFOUND)
                goto ferror;
            sg_error_clear(&ferr);
            e--;
        } else {
            /* Don't strip the extension if it's not a known
               extension.  */
            e = l;
        }
    }

    /* Try remaining extensions.  */
    memcpy(npath, path, e);
    if (attempt != IMG_PNG) {
        memcpy(npath + e, ".png", 5);
        r = sg_file_get(npath, 0, &fbuf, maxsize, &ferr);
        if (!r) {
            r = sg_pixbuf_loadpng(pbuf, &fbuf, err);
            goto havefile;
        }
        if (ferr->domain != &SG_ERROR_NOTFOUND)
            goto ferror;
        sg_error_clear(&ferr);
    }
    if (attempt != IMG_JPEG) {
        memcpy(npath + e, ".jpg", 5);
        r = sg_file_get(npath, 0, &fbuf, maxsize, &ferr);
        if (!r) {
            r = sg_pixbuf_loadpng(pbuf, &fbuf, err);
            goto havefile;
        }
        if (ferr->domain != &SG_ERROR_NOTFOUND)
            goto ferror;
        sg_error_clear(&ferr);
    }
    sg_error_notfound(err, path);
    return -1;

havefile:
    sg_buffer_destroy(&fbuf);
    return r;

ferror:
    sg_error_move(err, &ferr);
    return -1;
}
