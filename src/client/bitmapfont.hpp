#ifndef CLIENT_BITMAPFONT_HPP
#define CLIENT_BITMAPFONT_HPP
#include "texture.hpp"

class BitmapFont {
    Texture::Ref m_tex;

public:
    BitmapFont(const std::string &path);
    ~BitmapFont();

    void print(int x, int y, const char *text);
};

#endif
