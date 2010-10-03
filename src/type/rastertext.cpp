#include "rastertext.hpp"
#include "SDL_opengl.h"
#include <stdlib.h>

RasterText::RasterText()
    : text_(), font_(), alignment_(Left),
      textureLoaded_(false), texture_(0),
      vx1_(0), vx2_(0), vy1_(0), vy2_(0),
      tx1_(0), tx2_(0), ty1_(0), ty2_(0)
{ }

RasterText::~RasterText()
{ }

void RasterText::setText(std::string const &text)
{
    if (text == text_)
        return;
    text_ = text;
    textureLoaded_ = false;
}

void RasterText::setFont(Font const &font)
{
    font_ = font;
    textureLoaded_ = false;
}

void RasterText::setAlignment(RasterText::Alignment alignment)
{
    if (alignment_ == alignment)
        return;
    alignment_ = alignment;
    textureLoaded_ = false;
}

void RasterText::draw()
{
    glPushAttrib(GL_ENABLE_BIT);
    glEnable(GL_TEXTURE_2D);
    load();
    if (texture_) {
        glBindTexture(GL_TEXTURE_2D, texture_);
        glBegin(GL_TRIANGLE_STRIP);
        glTexCoord2f(tx1_, ty1_); glVertex2f(vx1_, vy1_);
        glTexCoord2f(tx2_, ty1_); glVertex2f(vx2_, vy1_);
        glTexCoord2f(tx1_, ty2_); glVertex2f(vx1_, vy2_);
        glTexCoord2f(tx2_, ty2_); glVertex2f(vx2_, vy2_);
        glEnd();
    }
    glPopAttrib();
}

void RasterText::load()
{
    if (textureLoaded_)
        return;
    if (text_.empty()) {
        if (texture_) {
            glDeleteTextures(1, &texture_);
            texture_ = 0;
        }
        textureLoaded_ = true;
        return;
    }
    void *data = NULL;
    unsigned int width = 0, height = 0;
    loadImage(&data, &width, &height);
    if (!texture_) {
        glGenTextures(1, &texture_);
        glBindTexture(GL_TEXTURE_2D, texture_);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    } else
        glBindTexture(GL_TEXTURE_2D, texture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, width, height, 0,
                 GL_ALPHA, GL_UNSIGNED_BYTE, data);
    free(data);
    textureLoaded_ = true;
}

void RasterText::unload()
{
    if (texture_)
        glDeleteTextures(1, &texture_);
}
