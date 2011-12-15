#ifndef GRAPHICS_TEXTURE_HPP
#define GRAPHICS_TEXTURE_HPP
#include <string>
#include "color.hpp"
#include "sys/sharedref.hpp"
#include "sys/resource.hpp"
#include "opengl.hpp"

class Texture : private Resource {
public:
    typedef SharedRef<Texture> Ref;

    Texture();
    virtual ~Texture();

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
    { Resource::setLoaded(false); }

    /* Const functions: callable through a Texture::Ref.  */
    virtual std::string name() const = 0;
    unsigned int width() const { return width_; }
    unsigned int height() const { return height_; }
    unsigned int twidth() const { return twidth_; }
    unsigned int theight() const { return theight_; }
    bool iscolor() const { return iscolor_; }
    bool hasalpha() const { return hasalpha_; }
    unsigned int refcount() const { return Resource::refcount(); }
    unsigned int tex() const { return tex_; }
    bool loaded() const { return Resource::loaded(); }

    /* This is safe to call even on NULL textures.  */
    void bind() const
    {
        if (this)
            glBindTexture(GL_TEXTURE_2D, tex_);
    }

    void incref() { Resource::incref(); }
    void decref() { Resource::decref(); }

protected:
    void registerTexture() { registerResource(); }
    /* Return false on failure.  */
    virtual bool loadTexture() = 0;

private:
    virtual void loadResource();
    virtual void unloadResource();
    virtual void markUnloaded();

    void *buf_;
    size_t bufsz_;
    unsigned int width_, height_;
    unsigned int twidth_, theight_;
    bool iscolor_, hasalpha_;
    unsigned int tex_;
    bool isnull_;
};

#endif
