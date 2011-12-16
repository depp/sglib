#include "rastertext.hpp"

// FIXME: unimplemented
Font::Font(Font const &f)
{ }

Font::~Font()
{ }

Font &Font::operator=(Font const &f)
{
    return *this;
}

bool RasterText::loadTexture()
{
    return true;
}
