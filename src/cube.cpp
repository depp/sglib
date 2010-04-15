#include "cube.hpp"
#include "SDL_opengl.h"

Cube::Cube(float x, float y, float size, const unsigned char color[3])
    : Object(x, y), size_(size)
{
    color_[0] = color[0];
    color_[1] = color[1];
    color_[2] = color[2];
}

Cube::~Cube()
{ }

void Cube::draw()
{
    float d = size_ * 0.5f;
    glPushAttrib(GL_CURRENT_BIT);
    glPushMatrix();
    setupMatrix();
    glTranslatef(0.0f, 0.0f, d);
    glColor3ubv(color_);
    glBegin(GL_LINES);
    for (int u = 0; u < 2; ++u) {
        for (int v = 0; v < 2; ++v) {
            glVertex3f( d, u ? d : -d, v ? d : -d);
            glVertex3f(-d, u ? d : -d, v ? d : -d);
            glVertex3f(u ? d : -d,  d, v ? d : -d);
            glVertex3f(u ? d : -d, -d, v ? d : -d);
            glVertex3f(u ? d : -d, v ? d : -d,  d);
            glVertex3f(u ? d : -d, v ? d : -d, -d);
        }
    }
    glEnd();
    glPopAttrib();
    glPopMatrix();
}
