#ifndef TYPE_FONT_PANGO_HPP
#define TYPE_FONT_PANGO_HPP
#include "font.hpp"
#include <pango/pango.h>

struct Font::Info {
    PangoFontDescription *desc;
    unsigned int refcount;

    Info()
        : desc(NULL), refcount(1)
    { }

    ~Info()
    {
        if (desc)
            pango_font_description_free(desc);
    }

    void retain()
    {
        if (this)
            refcount++;
    }

    void release()
    {
        if (this && !--refcount)
            delete this;
    }
};

#endif
