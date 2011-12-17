#include "actor.hpp"
#include "area.hpp"
#include "defs.hpp"
#include "client/opengl.hpp"
using namespace LD22;

Actor::~Actor()
{ }

void Actor::draw(int delta)
{
    int x = m_x0 + (m_x - m_x0) * delta / FRAME_TIME;
    int y = m_y0 + (m_y - m_y0) * delta / FRAME_TIME;
    int w = m_w, h = m_h;
    glColor3ub(255, 40, 40);
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x, y + h);
    glVertex2f(x + w, y + h);
    glVertex2f(x + w, y);
    glEnd();
    glColor3ub(255, 255, 255);
}

void Actor::advance()
{ }

bool Actor::wallAt(int x, int y)
{
    Area &a = *m_area;
    int s = Area::SCALE, w = m_w, h = m_h;
    for (int yp = y >> s; (yp << s) < y + h; ++yp) {
        for (int xp = x >> s; (xp << s) < x + w; ++xp) {
            int t = a.getTile(xp, yp);
            if (t)
                return true;
        }
    }
    return false;
}

bool Actor::moveTo(int x, int y)
{
    if (!wallAt(x, y)) {
        m_x = x;
        m_y = y;
        return false;
    } else {
        int x0 = m_x, y0 = m_y;
        int dx = x - x0, dy = y - y0;
        unsigned adx = dx > 0 ? dx : -dx;
        unsigned ady = dy > 0 ? dy : -dy;
        if (adx <= 1 && ady <= 1)
            return true;
        if (moveTo(x0 + dx/2, y0 + dy/2))
            return true;
        return moveTo(x, y);
    }
}
