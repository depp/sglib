#ifndef TYPE_FONT_OSX_HPP
#define TYPE_FONT_OSX_HPP
#include "font.hpp"
#include <CoreText/CoreText.h>

struct Font::Info {
    CTFontRef font;
    CFMutableDictionaryRef attrs;
    unsigned int refcount;

    Info()
        : font(NULL), attrs(NULL), refcount(1)
    { }

    ~Info()
    {
        if (font)
            CFRelease(font);
        if (attrs)
            CFRelease(attrs);
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
