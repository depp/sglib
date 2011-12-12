#include "font_osx.hpp"
#include <stdio.h>

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
{
    puts("setFamily");
    unsigned int i;
    mkinfo();
    AutoCF<CTFontRef> base, font;
    if (info_->font)
        base = info_->font;
    else
        base.set(CTFontCreateWithName(CFSTR("Helvetica"), 12.0f, NULL));
    for (i = 0; names[i]; ++i) {
        AutoCF<CFStringRef> name(CFStringCreateWithCString(
            NULL, names[i], kCFStringEncodingUTF8));
        font.set(CTFontCreateCopyWithFamily(base, 0.0f, NULL, name));
        if (font) {
            // FIXME not sure if this actually works...
            info_->font.moveFrom(font);
            return;
        }
    }
    info_->font.moveFrom(base);
}

void Font::setSize(float size)
{
    (void)size;
    /*
    AutoCF<CTFontRef> base, font;
    if (info_->font)
        base = info_->font;
    else
        base.set(CTFontCreateWithName(CFSTR("Helvetica"), 12.0f, NULL));
    */
}

void Font::setStyle(Style style)
{
    (void)style;
}

void Font::setVariant(Variant variant)
{
    (void)variant;
}

void Font::setWeight(int weight)
{
    (void)weight;
}

void Font::mkinfo()
{
    if (info_) {
        if (info_->refcount > 1) {
            Info *i = new Info;
            i->font = info_->font;
            info_->refcount--;
            info_ = i;
            return;
        }
        if (info_->font)
            return;
    } else
        info_ = new Info;
}
