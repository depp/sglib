#include "texturefile.hpp"
#include <stdlib.h>
#include <stdio.h>
#include <err.h>
#include <stdint.h>
#include <png.h>
#include <set>
#include <memory>
#include "SDL_opengl.h"

static const std::string filePrefix("file:");

struct TextureFileCompare {
    bool operator()(TextureFile *x, TextureFile *y)
    { return x->path() < y->path(); }
};

typedef std::set<TextureFile *, TextureFileCompare> TextureFileSet;
static TextureFileSet textureFiles;

Texture::Ref TextureFile::open(std::string const &path)
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

bool TextureFile::load()
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
    return false;
}

// FIXME error handling
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

    alloc(width, height, color, alpha);
    twidth = this->twidth();
    theight = this->theight();
    chan = channels();
    rowbytes = this->rowbytes();
    data = reinterpret_cast<unsigned char *>(buf());
    if (width * chan < rowbytes) {
        for (i = 0; i < height; ++i)
            memset(data + rowbytes * i + width * chan, 0,
                   rowbytes - width * chan);
    } else
        i = height;
    for (; i < theight; ++i)
        memset(data + rowbytes * i, 0, rowbytes);
    rows = reinterpret_cast<png_bytepp>(malloc(sizeof(*rows) * height));
    if (!rows) goto syserr;
    for (i = 0; i < height; ++i)
        rows[i] = data + rowbytes * i;
    png_read_image(pngp, rows);
    free(rows);
    rows = NULL;
    png_read_end(pngp, NULL);
    png_destroy_read_struct(&pngp, &infop, NULL);

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
    free(rows);
    return false;
}
