#include "texturefile.hpp"
#include <stdlib.h>
#include <stdio.h>
#include <err.h>
#include <stdint.h>
#include <png.h>
#include "SDL_opengl.h"

TextureFile *TextureFile::textureFile(std::string const &path)
{
    return new TextureFile(path);
}

TextureFile::TextureFile(std::string const &path)
    : path_(path), data_(0), width_(0), height_(0),
      twidth_(0), theight_(0), iscolor_(false), hasalpha_(false)
{ }

TextureFile::~TextureFile()
{
    free(data_);
}

static uint32_t
round_up_pow2(uint32_t x)
{
    x -= 1;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x += 1;
    return x;
}

bool TextureFile::loadPNG()
{
    png_structp pngp = NULL;
    png_infop infop = NULL;
    int depth, ctype;
    char const *path = path_.c_str();
    FILE *f = NULL;
    png_uint_32 width, height;
    uint32_t twidth, theight, i, rowbytes, chan;
    bool color = false, alpha = false;
    unsigned char *data = NULL;
    png_bytepp rows = NULL;

    f = fopen(path, "rb");
    if (!f) goto syserr;

    pngp = png_create_read_struct_2(
        PNG_LIBPNG_VER_STRING,
        NULL, NULL, NULL, NULL, NULL, NULL);
    if (!pngp) goto pngerr;
    infop = png_create_info_struct(pngp);
    if (!infop) goto pngerr;
    png_init_io(pngp, f);

    png_read_info(pngp, infop);
    png_get_IHDR(pngp, infop, &width, &height,
                 &depth, &ctype, NULL, NULL, NULL);
    twidth = round_up_pow2(width);
    theight = round_up_pow2(height);

    switch (ctype) {
    case PNG_COLOR_TYPE_PALETTE:
        color = true;
        png_set_palette_to_rgb(pngp);
        break;
    case PNG_COLOR_TYPE_GRAY:
        if (depth < 8)
            png_set_expand_gray_1_2_4_to_8(pngp);
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
        png_set_strip_16(pngp);

    chan = (color ? 3 : 1) + (alpha ? 1 : 0);
    rowbytes = (twidth * chan + 3) & ~3;
    data_ = data =
        reinterpret_cast<unsigned char *>(malloc(theight * rowbytes));
    if (width * chan < rowbytes) {
        for (i = 0; i < height; ++i)
            memset(data + rowbytes * i + width * chan, 0,
                   rowbytes - width * chan);
    } else
        i = height;
    for (; i < theight; ++i)
        memset(data + rowbytes * i, 0, rowbytes);
    if (!data) goto syserr;
    rows = reinterpret_cast<png_bytepp>(malloc(sizeof(*rows) * height));
    if (!rows) goto syserr;
    for (i = 0; i < height; ++i)
        rows[i] = data + rowbytes * i;
    png_read_image(pngp, rows);
    free(rows);
    rows = NULL;
    png_read_end(pngp, NULL);
    png_destroy_read_struct(&pngp, &infop, NULL);

    width_ = width;
    height_ = height;
    twidth_ = twidth;
    theight_ = theight;
    iscolor_ = color;
    hasalpha_ = hasalpha_;

    return true;
pngerr:
    fprintf(stderr, "Could not load texture '%s'", path);
    goto fail;
syserr:
    warn("Could not load texture '%s'", path);
    goto fail;
fail:
    if (f) fclose(f);
    png_destroy_read_struct(&pngp, &infop, NULL);
    free(data_);
    free(rows);
    data_ = NULL;
    return false;
}

void TextureFile::load()
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
    bool success;
    if (suffix == "png")
        success = loadPNG();
    else
        success = false;
    if (success) {
        GLuint tex = 0;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        GLint fmt;
        if (iscolor_)
            fmt = hasalpha_ ? GL_RGBA : GL_RGB;
        else
            fmt = hasalpha_ ? GL_LUMINANCE_ALPHA : GL_LUMINANCE;
        glTexImage2D(GL_TEXTURE_2D, 0, fmt, twidth_, theight_, 0,
                     fmt, GL_UNSIGNED_BYTE, data_);
        tex_ = tex;
    } else {
        fprintf(stderr, "Could not load image %s\n", path_.c_str());
        tex_ = Texture::getNull();
    }
}
