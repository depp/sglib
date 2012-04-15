#include "error.h"
#include "log.h"
#include "pixbuf.h"
#include "version.h"
#include <png.h>
#include <stdlib.h>

static void
sg_png_error(png_struct *pngp, const char *msg)
{
    struct sg_error **err = png_get_error_ptr(pngp);
    sg_logf(sg_logger_get("image"), LOG_ERROR, "LibPNG: %s", msg);
    sg_error_data(err, "PNG");
    longjmp(png_jmpbuf(pngp), 1);
}

static void
sg_png_warning(png_struct *pngp, const char *msg)
{
    (void) pngp;
    sg_logf(sg_logger_get("image"), LOG_WARN, "LibPNG: %s", msg);
}

struct sg_pngbuf {
    const char *p, *e;
};

static void
sg_png_read(png_struct *pngp, unsigned char *ptr, png_size_t len)
{
    struct sg_error **err;
    struct sg_pngbuf *b;
    size_t rem;
    b = png_get_io_ptr(pngp);
    rem = b->e - b->p;
    if (len <= rem) {
        memcpy(ptr, b->p, len);
        b->p += len;
    } else {
        sg_logs(sg_logger_get("image"), LOG_ERROR,
                "PNG: unexpected end of file");
        err = png_get_error_ptr(pngp);
        sg_error_data(err, "PNG");
        longjmp(png_jmpbuf(pngp), 1);
    }
}

int
sg_pixbuf_loadpng(struct sg_pixbuf *pbuf, const void *data, size_t len,
                  struct sg_error **err)
{
    png_struct *volatile pngp = NULL;
    png_info *volatile infop = NULL;
    struct sg_pngbuf rbuf;
    png_uint_32 width, height;
    unsigned i;
    int r, depth, ctype, ret, has_trns;
    sg_pixbuf_format_t pfmt;
    unsigned char **rowp;
    void *volatile tmp = NULL;

    rbuf.p = data;
    rbuf.e = rbuf.p + len;

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

    has_trns = png_get_valid(pngp, infop, PNG_INFO_tRNS) != 0;
    png_set_expand(pngp);
    switch (ctype) {
    case PNG_COLOR_TYPE_PALETTE:
        pfmt = has_trns ? SG_RGBA : SG_RGB;
        break;

    case PNG_COLOR_TYPE_GRAY:
        pfmt = has_trns ? SG_YA : SG_Y;
        break;

    case PNG_COLOR_TYPE_GRAY_ALPHA:
        pfmt = SG_YA;
        break;

    case PNG_COLOR_TYPE_RGB:
        pfmt = has_trns ? SG_RGBA : SG_RGB;
        break;

    case PNG_COLOR_TYPE_RGB_ALPHA:
        pfmt = SG_RGBA;
        break;

    default:
        sg_error_data(err, "PNG");
        sg_logf(sg_logger_get("image"), LOG_ERROR,
                "PNG has unknown color type");
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

    sg_pixbuf_premultiply_alpha(pbuf);

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

void
sg_version_libpng(struct sg_logger *lp)
{
    int v = png_access_version_number(), maj, min, mic;
    char vers[16];
    min = v / 100;
    mic = v % 100;
    maj = min / 100;
    min = min % 100;
    snprintf(vers, sizeof(vers), "%d.%d.%d", maj, min, mic);
    sg_version_lib(lp, "LibPNG", PNG_LIBPNG_VER_STRING, vers);
}
