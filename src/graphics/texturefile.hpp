#ifndef GRAPHICS_TEXTUREFILE_HPP
#define GRAPHICS_TEXTUREFILE_HPP
#include "texture.hpp"

class TextureFile : public Texture::Source {
public:
    static Texture::Ref open(std::string const &path)
    { return Texture::open(new TextureFile(path)); }
    virtual ~TextureFile();
    virtual bool load(Texture &tex);

private:
    TextureFile(std::string const &path);
    bool loadPNG(Texture &tex);
    std::string path_;
};

#endif
