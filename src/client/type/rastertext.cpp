#if 0

#include "rastertext.hpp"
#include "client/opengl.hpp"
#include <stdlib.h>
#include <stdio.h>

RasterText::Ref RasterText::create()
{
    RasterText::Ref r(new RasterText);
    r->registerTexture();
    return r;
}

RasterText::RasterText()
    : text_(), font_(), alignment_(Left),
      vx1_(0), vx2_(0), vy1_(0), vy2_(0),
      tx1_(0), tx2_(0), ty1_(0), ty2_(0)
{ }

RasterText::~RasterText()
{ }

std::string RasterText::name() const
{
    char buf[32];
#if !defined(_WIN32)
    snprintf(buf, sizeof(buf), "<rastertext %p>", (void *)this);
#else
    _snprintf(buf, sizeof(buf), "<rastertext %p>", (void *) this);
    buf[sizeof(buf) - 1] = '\0';
#endif
    return std::string(buf);
}

void RasterText::setText(std::string const &text)
{
    if (text == text_)
        return;
    text_ = text;
    markDirty();
}

void RasterText::setFont(Font const &font)
{
    font_ = font;
    markDirty();
}

void RasterText::setAlignment(RasterText::Alignment alignment)
{
    if (alignment_ == alignment)
        return;
    alignment_ = alignment;
    markDirty();
}

void RasterText::draw()
{
    glPushAttrib(GL_ENABLE_BIT);
    glEnable(GL_TEXTURE_2D);
    bind();
    glBegin(GL_TRIANGLE_STRIP);
    glTexCoord2f(tx1_, ty1_); glVertex2f(vx1_, vy1_);
    glTexCoord2f(tx2_, ty1_); glVertex2f(vx2_, vy1_);
    glTexCoord2f(tx1_, ty2_); glVertex2f(vx1_, vy2_);
    glTexCoord2f(tx2_, ty2_); glVertex2f(vx2_, vy2_);
    glEnd();
    glPopAttrib();
}

#endif
