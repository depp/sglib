#ifndef GRAPHICS_TEXTURE_HPP
#define GRAPHICS_TEXTURE_HPP
#include <string>
#include "color.hpp"

class Texture {
    friend class Ref;
public:
    class Ref {
    public:
        Ref();
        ~Ref() { ptr_->refcount_--; }
        explicit Ref(Texture *t) : ptr_(t) { ptr_->refcount_++; }
        Ref(Ref const &r) : ptr_(r.ptr_) { ptr_->refcount_++; }
        Ref &operator=(Ref const &r) {
            ptr_->refcount_--;
            (ptr_ = r.ptr_)->refcount_++;
            return *this;
        }
        Texture const *operator->() const { return ptr_; }

    private:
        Texture *ptr_;
    };

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

    /* Const functions: callable through a Texture::Ref.  */
    virtual std::string name() const = 0;
    bool width() const { return width_; }
    bool height() const { return height_; }
    bool twidth() const { return twidth_; }
    bool theight() const { return theight_; }
    bool iscolor() const { return iscolor_; }
    bool hasalpha() const { return hasalpha_; }
    unsigned int refcount() const { return refcount_; }
    unsigned int tex() const { return tex_; }
    bool loaded() const { return loaded_; }

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
