#include "texture.hpp"
#include "dummytexture.hpp"
#include "SDL_opengl.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

Texture::Texture()
    : buf_(0), bufsz_(0), width_(0), height_(0), twidth_(0), theight_(0),
      iscolor_(false), hasalpha_(false), tex_(0), isnull_(false)
{ }

Texture::~Texture()
{
    free(buf_);
}

void Texture::loadResource()
{
    if (loaded())
        return;
    if (tex_)
        fprintf(stderr, "Reloading texture %s\n", name().c_str());
    else
        fprintf(stderr, "Loading texture %s\n", name().c_str());
    bool success;
    try {
        success = loadTexture();
    } catch (std::exception const &exc) {
        fprintf(stderr, "Error: %s\n", exc.what());
        success = false;
    }
    if (success) {
        GLuint tex = 0;
        if (tex_ && !isnull_)
            tex = tex_;
        else
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
                     fmt, GL_UNSIGNED_BYTE, buf_);
        tex_ = tex;
        setLoaded(true);
        isnull_ = false;
    } else {
        fprintf(stderr, "    failed\n");
        unloadResource();
        // Make sure to set loaded_ true before checking if the dummy
        // texture is loaded, because this might be the dummy texture.
        setLoaded(true);
        isnull_ = true;
        Texture &t = DummyTexture::nullTexture;
        if (!t.loaded())
            t.loadResource();
        tex_ = t.tex_;
    }
}

void Texture::unloadResource()
{
    if (tex_ != 0 && !isnull_) {
        fprintf(stderr, "Unloading texture %s\n", name().c_str());
        GLuint tex = tex_;
        glDeleteTextures(1, &tex);
    }
    tex_ = 0;
    setLoaded(false);
    isnull_ = false;
}

unsigned int
round_up_pow2(unsigned int x)
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

void Texture::alloc(unsigned int width, unsigned int height,
                    bool iscolor, bool hasalpha)
{
    // FIXME overflow, error handling
    unsigned int
        twidth = round_up_pow2(width), theight = round_up_pow2(height),
        chan = (iscolor ? 3 : 1) + (hasalpha ? 1 : 0),
        rb = (twidth * chan + 3) & ~3;
    size_t sz = (size_t)rb * theight;
    if (bufsz_ != sz) {
        void *buf = malloc(sz);
        if (!buf) throw std::bad_alloc();
        if (buf_) free(buf_);
        buf_ = buf;
        bufsz_ = sz;
    }
    width_ = width;
    height_ = height;
    twidth_ = twidth;
    theight_ = theight;
    iscolor_ = iscolor;
    hasalpha_ = hasalpha;
}
