#ifndef CLIENT_DUMMYTEXTURE_HPP
#define CLIENT_DUMMYTEXTURE_HPP
#include "texture.hpp"
#include "color.hpp"

/* A dummy texture is a simple checkerboard pattern.  */
class DummyTexture : public Texture {
public:
    DummyTexture(std::string const &name, Color c1, Color c2)
        : name_(name), c1_(c1), c2_(c2)
    { }
    virtual ~DummyTexture();
    virtual std::string name() const;

    static DummyTexture nullTexture;

protected:
    virtual bool loadTexture();

private:
    std::string name_;
    Color c1_, c2_;
};

#endif
