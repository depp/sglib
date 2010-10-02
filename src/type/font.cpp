#include "font_pango.hpp"
#include <math.h>
#include <assert.h>

Font::Font(Font const &f)
    : info_(f.info_)
{
    if (info_)
        info_->refcount++;
}

Font::~Font()
{
    if (!--info_->refcount)
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
    mkinfo();
    PangoFontDescription *desc = info_->desc;
    pango_font_description_set_family(desc, names[0]);
}

void Font::setSize(float size)
{
    mkinfo();
    PangoFontDescription *desc = info_->desc;
    pango_font_description_set_size(
        desc, (gint)floor(PANGO_SCALE * size + 0.5f));
}

void Font::mkinfo()
{
    if (info_) {
        if (info_->refcount > 1) {
            Info *i = new Info;
            i->desc = pango_font_description_copy(info_->desc);
            assert(i->desc); // FIXME exception
            info_->refcount--;
            info_ = i;
            return;
        }
        if (info_->desc)
            return;
    } else
        info_ = new Info;
    info_->desc = pango_font_description_new();
    assert(info_->desc); // FIXME exception
}
