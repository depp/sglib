#ifndef GRAPHICS_TEXTURE_HPP
#define GRAPHICS_TEXTURE_HPP
#include <string>
#include "color.hpp"
#include "sys/sharedref.hpp"
#include "SDL_opengl.h"

class Texture {
public:
    typedef SharedRef<Texture> Ref;

    Texture();
    virtual ~Texture();

    /* Load all textures with active references, unload all textures
       without active references.  */
    static void updateAll();

    /* Non-const functions: used by subclasses of Texture to fill the
       texture buffer in the "load" method.  */
    void *buf() { return buf_; }
    void alloc(unsigned int width, unsigned int height,
               bool iscolor, bool hasalpha);
    unsigned int channels()
    { return (iscolor_ ? 3 : 1) + (hasalpha_ ? 1 : 0); }
    unsigned int rowbytes()
    { return (twidth_ * channels() + 3) & ~3; }
    void markDirty()
    { loaded_ = false; }

    /* Const functions: callable through a Texture::Ref.  */
    virtual std::string name() const = 0;
    unsigned int width() const { return width_; }
    unsigned int height() const { return height_; }
    unsigned int twidth() const { return twidth_; }
    unsigned int theight() const { return theight_; }
    bool iscolor() const { return iscolor_; }
    bool hasalpha() const { return hasalpha_; }
    unsigned int refcount() const { return refcount_; }
    unsigned int tex() const { return tex_; }
    bool loaded() const { return loaded_; }

    /* This is safe to call even on NULL textures.  */
    void bind() const
    {
        if (this)
            glBindTexture(GL_TEXTURE_2D, tex_);
    }

    void incref()
    {
        refcount_++;
    }

    void decref()
    {
        refcount_--;
    }

protected:
    /* Register texture to be loaded and unloaded in updateAll.  The
       texture may be deleted by updateAll.  */
    void registerTexture();
    /* Return false on failure.  */
    virtual bool load() = 0;

private:
    void loadTex();
    void unloadTex();
    Texture(Texture const &);
    Texture &operator=(Texture const &);

    void *buf_;
    size_t bufsz_;
    unsigned int width_, height_;
    unsigned int twidth_, theight_;
    bool iscolor_, hasalpha_;
    unsigned int refcount_;
    unsigned int tex_;
    bool loaded_, isnull_, registered_;
};

#endif
