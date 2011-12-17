#include "actor.hpp"
#include "client/opengl.hpp"
using namespace LD22;

Actor::~Actor()
{ }

void Actor::draw()
{
    int x = m_x, y = m_y;
    glColor3ub(255, 40, 40);
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x, y + 32);
    glVertex2f(x + 32, y + 32);
    glVertex2f(x + 32, y);
    glEnd();
    glColor3ub(255, 255, 255);
}

void Actor::advance()
{ }
