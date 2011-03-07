#ifndef GRAPHICS_TEXTUREFILE_HPP
#define GRAPHICS_TEXTUREFILE_HPP
#include "texture.hpp"

class TextureFile : public Texture {
public:
    typedef Texture::RefT<TextureFile> Ref;

    static Ref open(std::string const &path);
    virtual ~TextureFile();
    virtual std::string name() const;
    std::string const &path() const { return path_; }

protected:
    virtual bool load();

private:
    TextureFile(std::string const &path);
    bool loadPNG();
    std::string path_;
};

#endif
