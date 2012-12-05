/* Copyright 2012 Dietrich Epp <depp@zdome.net> */
#ifndef SGPP_BITMAPFONT_HPP
#define SGPP_BITMAPFONT_HPP
#include "texture.hpp"

class BitmapFont {
    Texture::Ref m_tex;

public:
    BitmapFont(const char *path);
    ~BitmapFont();

    void print(int x, int y, const char *text);
};

#endif