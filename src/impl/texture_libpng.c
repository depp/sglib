#include "texture.h"

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

struct sg_pngbuf {
    const char *p, *e;
};

static void
sg_pngread(png_structp pngp, png_bytep ptr, png_size_t len)
{
    struct sg_pngbuf *b;
    size_t rem;
    b = png_get_io_ptr(pngp);
    rem = b->e - b->p;
    if (len <= rem) {
        memcpy(ptr, b->p, len);
        b->p += len;
    } else {
        // error
        longjmp(png_jmpbuf(pngp), 1);
    }
}

void
sg_texbuf_loadpng(struct sg_pixbuf *buf)
{
    png_structp pngp = NULL;
    png_infop infop = NULL;
    struct sg_buffer fbuf;
    struct sg_texbuf tbuf;
    struct sg_pngbuf pbuf;
    struct sg_error *err = NULL;
    unsigned width, height;
    int r, depth, ctype;
    int color, alpha;

    r = sg_file_get(PATH, 0, &buf, -1, &err);
    if (r)
        goto error;
    pbuf.p = fbuf.data;
    pbuf.e = pbuf.p + fbuf.length;

/*
    int depth, ctype;
    png_uint_32 width, height;
    uint32_t th, i, rowbytes, chan;
    bool color = false, alpha = false;
    unsigned char *data = NULL;
    std::vector<png_bytep> rows;

    File f(path_.c_str(), 0);
*/

    pngp = png_create_read_struct_2(
        PNG_LIBPNG_VER_STRING,
        NULL, NULL, NULL, NULL, NULL, NULL);
    if (!pngp)
        goto error;
    infop = png_create_info_struct(pngp);
    if (!infop)
        goto error;

    if (setjmp(png_jmpbuf(pngp)))
        goto error;

    png_set_read_fn(pngp, &pbuf, sg_pngread);

    png_read_info(pngp, infop);
    png_get_IHDR(pngp, infop, &width, &height, &depth, &ctype,
                 NULL, NULL, NULL);

    switch (ctype) {
    case PNG_COLOR_TYPE_PALETTE:
        color = 1;
        alpha = ??;
        png_set_palette_to_rgb(ps.png);
        break;

    case PNG_COLOR_TYPE_GRAY:
        color = 0;
        alpha = 0;
        if (depth < 8)
            png_set_expand_gray_1_2_4_to_8(ps.png);
        break;

    case PNG_COLOR_TYPE_GRAY_ALPHA:
        color = 0;
        alpha = 1;
        break;

    case PNG_COLOR_TYPE_RGB:
        color = 1;
        alpha = 0;
        break;

    case PNG_COLOR_TYPE_RGB_ALPHA:
        color = 1;
        alpha = 1;
        break;

    default:
        goto error;
    }

    if (depth > 8)
        png_set_strip_16(ps.png);

    sg_texbuf_init(&tbuf, tex);
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
