#include "texture.hpp"
#include "SDL_opengl.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <set>

Texture::Source::~Source() { }

Texture::DummySource::~DummySource()
{ }

bool Texture::DummySource::load(Texture &tex)
{
    unsigned char *buf, *p, c1[3], c2[3];
    unsigned int w = 16, h = 16, rowbytes, x, y;
    memcpy(c1, c1_.c, sizeof(c1));
    memcpy(c2, c2_.c, sizeof(c2));
    tex.alloc(w, h, true, false);
    buf = reinterpret_cast<unsigned char *>(tex.buf());
    rowbytes = tex.rowbytes();
    for (y = 0; y < h / 2; ++y) {
        p = buf + y * rowbytes;
        for (x = 0; x < w / 2; ++x) {
            p[x*3+0] = c1[0];
            p[x*3+1] = c1[1];
            p[x*3+2] = c1[2];
        }
        for (; x < w; ++x) {
            p[x*3+0] = c2[0];
            p[x*3+1] = c2[1];
            p[x*3+2] = c2[2];
        }
    }
    for (; y < h; ++y) {
        p = buf + y * rowbytes;
        for (x = 0; x < w / 2; ++x) {
            p[x*3+0] = c2[0];
            p[x*3+1] = c2[1];
            p[x*3+2] = c2[2];
        }
        for (; x < w; ++x) {
            p[x*3+0] = c1[0];
            p[x*3+1] = c1[1];
            p[x*3+2] = c1[2];
        }
    }
    return true;
}

struct TextureCompare {
    bool operator()(Texture *x, Texture *y)
    { return x->name() < y->name(); }
};

typedef std::set<Texture *, TextureCompare> TextureSet; 
static TextureSet textures;

Texture::Ref Texture::open(Texture::Source *src)
{
    Texture *t = 0;
    try {
        t = new Texture(src);
        std::pair<TextureSet::iterator, bool> r = textures.insert(t);
        if (!r.second)
            delete src;
        src = 0;
        return Ref(*r.first);
    } catch (...) {
        if (t)
            delete t;
        if (src)
            delete src;
        throw;
    }
}

void Texture::load()
{
    if (loaded_)
        fprintf(stderr, "Reloading texture %s\n", name().c_str());
    else
        fprintf(stderr, "Loading texture %s\n", name().c_str());
    bool success = false;
    success = src_->load(*this);
    if (success) {
        GLuint tex = 0;
        if (loaded_)
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
        loaded_ = true;
    } else {
        fprintf(stderr, "    failed\n");
        unload();
        tex_ = nullTexture.tex_;
    }
}

void Texture::unload()
{
    if (!loaded_)
        return;
    fprintf(stderr, "Unloading texture %s\n", name().c_str());
    GLuint tex = tex_;
    glDeleteTextures(1, &tex);
    tex_ = 0;
    loaded_ = false;
}

void Texture::updateAll()
{
    if (!nullTexture.loaded_) {
        nullTexture.load();
        if (!nullTexture.loaded_) {
            fprintf(stderr, "Could not load null texture!");
            nullTexture.loaded_ = true;
        }
    }
    TextureSet::iterator i = textures.begin(), e = textures.end(), c;
    while (i != e) {
        c = i;
        ++i;
        Texture *t = *c;
        if (t->refcount_ > 0) {
            if (!t->tex_)
                t->load();
        } else {
            t->unload();
            textures.erase(c);
            delete t->src_;
            delete t;
        }
    }
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
        rb = (width * chan + 3) & ~3;
    void *buf = malloc(rb * height);
    if (!buf) throw std::bad_alloc();
    if (buf_) free(buf);
    buf_ = buf;
    width_ = width;
    height_ = height;
    twidth_ = twidth;
    theight_ = theight;
    iscolor_ = iscolor;
    hasalpha_ = hasalpha;
}

Texture::Texture(Source *src)
    : src_(src), buf_(0), width_(0), height_(0),
      twidth_(0), theight_(0), iscolor_(false), hasalpha_(false),
      refcount_(0), tex_(0), loaded_(false)
{ }

Texture::~Texture()
{
    free(buf_);
}

static Texture::DummySource nullSource
("<null>", Color(255, 0, 255), Color(32, 32, 32));

Texture Texture::nullTexture(&nullSource);
