#include "area.hpp"
#include "actor.hpp"
#include "client/opengl.hpp"
#include <cstring>
using namespace LD22;

Area::Area()
{
    std::memset(m_tile, 0, sizeof(m_tile));
    for (int i = 1; i <= 5; ++i)
        m_tile[1][i] = 1;
}

Area::~Area()
{ }

void Area::addActor(Actor *a)
{
    m_actors.push_back(a);
    a->m_area = this;
    a->m_x0 = a->m_x;
    a->m_y0 = a->m_x;
}

void Area::draw(int delta)
{
    glBegin(GL_QUADS);
    for (int y = 0; y < HEIGHT; ++y) {
        for (int x = 0; x < WIDTH; ++x) {
            int t = m_tile[y][x];
            if (t) {
                glVertex2f(x, y);
                glVertex2f(x, y + 1);
                glVertex2f(x + 1, y + 1);
                glVertex2f(x + 1, y);
            }
        }
    }
    glEnd();

    glScalef(1.0f/32.0f, 1.0f/32.0f, 1.0f);
    std::vector<Actor *>::iterator i = m_actors.begin(), e = m_actors.end();
    for (; i != e; ++i) {
        Actor &a = **i;
        a.draw(delta);
    }
}

void Area::advance()
{
    std::vector<Actor *>::iterator i = m_actors.begin(), e = m_actors.end();
    for (; i != e; ++i) {
        Actor &a = **i;
        a.m_x0 = a.m_x;
        a.m_y0 = a.m_y;
        a.advance();
    }
}
