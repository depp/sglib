#include "opengl.hpp"
#include "ship.hpp"
#include "shapes.hpp"
#include <math.h>
namespace Space {

Ship::Ship()
  : angle(0.0f), angular_velocity(0.0f), direction(), velocity()
{ }

Ship::~Ship()
{ }

void Ship::move(World &, double delta)
{
    location += velocity * delta;
    angle += angular_velocity * delta;
    direction.v[0] = cosf(angle);
    direction.v[1] = sinf(angle);
}

void Ship::draw()
{
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glTranslatef(location.v[0], location.v[1], 0.0f);
    glRotatef(angle * 180.0f * M_1_PI, 0.0f, 0.0f, 1.0f);
    
    glShadeModel(GL_SMOOTH);
    glBegin(GL_TRIANGLES);
    glColor3ub(119, 119, 119);
    glVertex2f(-16.0f, 8.0f);
    glVertex2f(-16.0f, -8.0f);
    glColor3ub(255, 0, 0);
    glVertex2f(16.0f, 0.0f);
    glEnd();
    
    glPushMatrix();
    glScalef(22.0f, 22.0f, 1.0f);
    glShadeModel(GL_FLAT);
    glColor3ub(255, 0, 0);
    glBegin(GL_LINE_LOOP);
    drawCircle();
    glEnd();
    glPopMatrix();
    
    glPopMatrix();
}

}
