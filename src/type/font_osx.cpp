#include "font_osx.hpp"

Font::Font(Font const &f)
    : info_(f.info_)
{
    if (info_)
        info_->refcount++;
}

Font::~Font()
{
    if (info_ && !--info_->refcount)
        delete info_;
}

Font &Font::operator=(Font const &f)
{
    Info *old = info_;
    info_ = f.info_;
    info_->retain();
    old->release();
    return *this;
}

void Font::setFamily(char const *const names[])
{ }

void Font::setSize(float size)
{ }

void Font::setStyle(Style style)
{ }

void Font::setVariant(Variant variant)
{ }

void Font::setWeight(int weight)
{ }

void Font::mkinfo()
{ }
