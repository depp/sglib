#ifndef GRAPHICS_TEXTURE_HPP
#define GRAPHICS_TEXTURE_HPP
#include <string>
#include "color.hpp"

class Texture {
    friend class Ref;
public:
    class Source {
    public:
        /* The name for each source should be unique.  Two sources
           with the same name will be combined into one.  */
        Source(std::string const &name) : name_(name) { }
        virtual ~Source();
        /* Load data into the given texture.  Return false if there is
           an error.  */
        virtual bool load(Texture &tex) = 0;
        std::string const &name() const { return name_; }
    private:
        std::string name_;
        Source(Source const &);
        Source &operator=(Source const &);
    };

    class DummySource : public Source {
    public:
        DummySource(std::string const &name, Color c1, Color c2)
            : Source(name), c1_(c1), c2_(c2)
        { }
        virtual ~DummySource();
        virtual bool load(Texture &tex);
    private:
        Color c1_, c2_;
    };
        

    class Ref {
    public:
        Ref() : ptr_(&nullTexture) { ptr_->refcount_++; }
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

    static Ref open(Source *src);
    static void updateAll();

    /* Non-const functions: used by subclasses of Texture::Source to
       fill the texture buffer in the "load" method.  */
    void *buf() { return buf_; }
    void alloc(unsigned int width, unsigned int height,
               bool iscolor, bool hasalpha);
    unsigned int channels()
    { return (iscolor_ ? 3 : 1) + (hasalpha_ ? 1 : 0); }
    unsigned int rowbytes()
    { return (twidth_ * channels() + 3) & ~3; }

    /* Const functions: callable through a Texture::Ref.  */
    std::string const &name() const { return src_->name(); }
    bool width() const { return width_; }
    bool height() const { return height_; }
    bool twidth() const { return twidth_; }
    bool theight() const { return theight_; }
    bool iscolor() const { return iscolor_; }
    bool hasalpha() const { return hasalpha_; }
    unsigned int refcount() const { return refcount_; }
    unsigned int tex() const { return tex_; }
    bool loaded() const { return loaded_; }

private:
    void load();
    void unload();
    Texture(Source *src);
    ~Texture();
    Texture(Texture const &);
    Texture &operator=(Texture const &);

    static Texture nullTexture;

    Source *src_;
    void *buf_;
    unsigned int width_, height_;
    unsigned int twidth_, theight_;
    bool iscolor_, hasalpha_;
    unsigned int refcount_;
    unsigned int tex_; // set nonzero on success or failure
    bool loaded_; // set true only on success
};

#endif
