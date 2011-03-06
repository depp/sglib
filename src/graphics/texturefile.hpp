#ifndef GRAPHICS_TEXTUREFILE_HPP
#define GRAPHICS_TEXTUREFILE_HPP
#include "texture.hpp"
#include <string>

class TextureFile : public Texture::Source {
public:
    static TextureFile *textureFile(std::string const &path);
    virtual void load();

private:
    bool loadPNG();

    std::string path_;
    unsigned char *data_;
    unsigned int width_, height_;
    unsigned int twidth_, theight_;
    bool iscolor_, hasalpha_;

    TextureFile(std::string const &path);
    ~TextureFile();
    TextureFile(TextureFile const &);
    TextureFile &operator=(TextureFile const &);
};

#endif
