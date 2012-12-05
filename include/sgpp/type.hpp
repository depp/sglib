/* Copyright 2012 Dietrich Epp <depp@zdome.net> */
#ifndef SGPP_TYPE_HPP
#define SGPP_TYPE_HPP
#include "sg/type.h"
#include <stddef.h>

class TextLayout {
    sg_layout *m_p;

    TextLayout(const TextLayout &);
    TextLayout &operator=(const TextLayout &);

public:
    TextLayout()
        : m_p(0)
    {
        m_p = sg_layout_new();
    }

    ~TextLayout()
    {
        sg_layout_decref(m_p);
    }

    void setText(const char *text, size_t length)
    {
        sg_layout_settext(m_p, text, length);
    }

    void draw()
    {
        sg_layout_draw(m_p);
    }
};

#endif
