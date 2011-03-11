#include "texturefile.hpp"
#include "sys/ifile.hpp"
#include "sys/path.hpp"
#include <stdlib.h>
#include <stdio.h>
#include <err.h>
#include <stdint.h>
#include <png.h>
#include <set>
#include <memory>
#include <vector>
#include "SDL_opengl.h"

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
        IFile *f = reinterpret_cast<IFile *>(png_get_io_ptr(pngp));
        size_t pos = 0;
        while (pos < length) {
            size_t amt = f->read(data + pos, length - pos);
            if (amt == 0) {
                fputs("Unexpected end of file\n", stderr);
                longjmp(png_jmpbuf(pngp), 1);
            }
            pos += amt;
        }
    } catch (std::exception const &e) {
        fprintf(stderr, "Error: %s\n", e.what());
        longjmp(png_jmpbuf(pngp), 1);
    } catch (...) {
        fputs("Unknown error\n", stderr);
        longjmp(png_jmpbuf(pngp), 1);
    }
}

// FIXME error handling
bool TextureFile::loadPNG()
{
    std::auto_ptr<IFile> f;
    PNGStruct ps;
    int depth, ctype;
    png_uint_32 width, height;
    uint32_t twidth, theight, i, rowbytes, chan;
    bool color = false, alpha = false;
    unsigned char *data = NULL;
    std::vector<png_bytep> rows;

    f.reset(Path::openIFile(path_));

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

    png_set_read_fn(ps.png, f.get(), textureFileRead);

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
    rows.resize(height);
    for (i = 0; i < height; ++i)
        rows[i] = data + rowbytes * i;
    png_read_image(ps.png, &rows.front());
    png_read_end(ps.png, NULL);
    return true;
}
