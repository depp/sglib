/* Copyright 2012 Dietrich Epp <depp@zdome.net> */
#include "sg/opengl.h"
#include "sgpp/viewport.hpp"

Viewport::Viewport(int x, int y, int width, int height)
    : m_x(x), m_y(y), m_width(width), m_height(height),
      m_parent(0)
{
    glViewport(x, y, width, height);
}

Viewport::Viewport(Viewport &v, int x, int y, int width, int height)
    : m_x(x + v.m_x), m_y(y + v.m_y), m_width(width), m_height(height),
      m_parent(&v)
{
    /*
    if (x < 0 || x > v.m_width ||
        y < 0 || y > v.m_height ||
        width < 0 || width > v.m_width - x ||
        height < 0 || height > v.m_height - y)
    */
    glViewport(m_x, m_y, m_width, m_height);
}

Viewport::~Viewport()
{
    if (m_parent)
        glViewport(m_parent->m_x, m_parent->m_y,
                   m_parent->m_width, m_parent->m_height);
}

void Viewport::ortho() const
{
    glOrtho(0, m_width, 0, m_height, -1, 1);
}
