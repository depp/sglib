#ifndef TYPE_FONT_OSX_HPP
#define TYPE_FONT_OSX_HPP
#include "font.hpp"
#include "sys/autocf_osx.hpp"
#include <CoreText/CoreText.h>

struct Font::Info {
    AutoCF<CTFontRef> font;
    AutoCF<CFMutableDictionaryRef> attrs;
    unsigned int refcount;

    Info()
        : refcount(1)
    { }

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
