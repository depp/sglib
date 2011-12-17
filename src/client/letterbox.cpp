#include "letterbox.hpp"
#include "opengl.hpp"

void Letterbox::setOSize(int w, int h)
{
    m_ow = w;
    m_oh = h;
    m_x = -1;
}

void Letterbox::setISize(int w, int h)
{
    m_iw = w;
    m_ih = h;
    m_x = -1;
}

void Letterbox::enable()
{
    if (m_x < 0)
        recalc();

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, m_ow, 0, m_oh, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    if (m_w < m_ow) {
        int x0 = 0, x1 = m_x, x2 = m_x + m_w, x3 = m_ow;
        int y0 = 0, y1 = m_oh;
        glColor3ub(50, 50, 50);

        glBegin(GL_QUADS);
        glVertex2s(x0, y0);
        glVertex2s(x0, y1);
        glVertex2s(x1, y1);
        glVertex2s(x1, y0);
        glVertex2s(x2, y0);
        glVertex2s(x2, y1);
        glVertex2s(x3, y1);
        glVertex2s(x3, y0);
        glEnd();

        glColor3ub(255,255,255);
    } else if (m_h < m_oh) {
        int x0 = 0, x1 = m_ow;
        int y0 = 0, y1 = m_y, y2 = m_y + m_h, y3 = m_oh;
        glColor3ub(50, 50, 50);

        glBegin(GL_QUADS);
        glVertex2s(x0, y0);
        glVertex2s(x0, y1);
        glVertex2s(x1, y1);
        glVertex2s(x1, y0);
        glVertex2s(x0, y2);
        glVertex2s(x0, y3);
        glVertex2s(x1, y3);
        glVertex2s(x1, y2);
        glEnd();

        glColor3ub(255,255,255);
    }

    glEnable(GL_SCISSOR_TEST);
    glViewport(m_x, m_y, m_w, m_h);
    glScissor(m_x, m_y, m_w, m_h);
}

void Letterbox::disable()
{
    glDisable(GL_SCISSOR_TEST);
    glViewport(0, 0, m_ow, m_oh);
}

void Letterbox::recalc()
{
    if (m_ow * m_ih > m_oh * m_iw) {
        // Output window is wider
        m_y = 0;
        m_h = m_oh;
        m_w = m_oh * m_iw / m_ih;
        m_x = (m_ow - m_w) / 2;
    } else {
        // Output window is not wider
        m_x = 0;
        m_w = m_ow;
        m_h = m_ow * m_ih / m_iw;
        m_y = (m_oh - m_h) / 2;
    }
}
