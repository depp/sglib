/* Copyright 2012 Dietrich Epp <depp@zdome.net> */
#ifndef SGPP_TEXTURE_HPP
#define SGPP_TEXTURE_HPP
#include "sg/texture.h"
#include "sg/opengl.h"
#include "sgpp/sharedref.hpp"

class Texture {
    template<class T> friend class SharedRef;

protected:
    sg_texture_image *m_ptr;

    Texture() : m_ptr(0) { }
    Texture(sg_texture_image *ptr) : m_ptr(ptr) { }

    void incref() { if (m_ptr) sg_resource_incref(&m_ptr->r); }
    void decref() { if (m_ptr) sg_resource_decref(&m_ptr->r); }
    operator bool() const { return m_ptr != 0; }

public:
    typedef SharedRef<Texture> Ref;

    bool loaded() const { return m_ptr && m_ptr->texnum != 0; }
    unsigned iwidth() const { return m_ptr->iwidth; }
    unsigned iheight() const { return m_ptr->iheight; }
    unsigned twidth() const { return m_ptr->twidth; }
    unsigned theight() const { return m_ptr->theight; }

    void bind() const
    {
        glBindTexture(GL_TEXTURE_2D, m_ptr->texnum);
    }

    static Ref file(const char *path);
};

#endif
