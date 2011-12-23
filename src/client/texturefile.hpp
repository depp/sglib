#ifndef CLIENT_TEXTUREFILE_HPP
#define CLIENT_TEXTUREFILE_HPP
#include "texture.hpp"

class TextureFile : public Texture {
public:
    typedef SharedRef<TextureFile> Ref;

    static Ref open(std::string const &path);
    virtual ~TextureFile();
    virtual std::string name() const;
    std::string const &path() const { return path_; }

protected:
    virtual bool loadTexture();

private:
    TextureFile(std::string const &path);
    bool loadPNG();
    bool loadJPEG();
    std::string path_;
};

#endif
