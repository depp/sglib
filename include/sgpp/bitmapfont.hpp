/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
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
