#include "actor.hpp"
#include "area.hpp"
#include "client/opengl.hpp"
using namespace LD22;

Actor::~Actor()
{ }

void Actor::draw()
{
    int x = m_x, y = m_y, w = m_w, h = m_h;
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

bool Actor::canMove(int x, int y)
{
    Area &a = *m_area;
    int s = Area::SCALE, w = m_w, h = m_h;
    for (int yp = y >> s; (yp << s) < y + h; ++yp) {
        for (int xp = x >> s; (xp << s) < x + w; ++xp) {
            int t = a.getTile(xp, yp);
            if (t)
                return false;
        }
    }
    return true;
}
