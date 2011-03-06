#ifndef GRAPHICS_TEXTURE_HPP
#define GRAPHICS_TEXTURE_HPP

class Texture {
public:
    class Source {
    public:
        Source() : refcount_(0), tex_(0) { }
        virtual void load() = 0;
        void retain() { refcount_++; }
        void release() { refcount_--; }
        unsigned int refcount_;
        unsigned int tex_;
    };

    Texture()
        : src_(&nullSrc)
    {
        src_->retain();
    }

    explicit Texture(Source *s)
        : src_(s)
    {
        src_->retain();
    }

    Texture(Texture const &t)
        : src_(t.src_)
    {
        src_->retain();
    }

    ~Texture()
    {
        src_->release();
    }

    Texture &operator=(Texture const &t)
    {
        Source *sn = t.src_, *so = src_;
        sn->retain();
        so->release();
        src_ = sn;
        return *this;
    }

    void load()
    {
        if (!src_->tex_)
            src_->load();
    }

    unsigned int get() const
    {
        return src_->tex_;
    }

    static unsigned int getNull()
    {
        return nullSrc.tex_;
    }

private:
    class NullSource : public Source {
        virtual void load();
    };

    static NullSource nullSrc;

    Source *src_;
};

#endif
