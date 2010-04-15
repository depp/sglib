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

const GLshort kCubeVertices[8][3] = {
    { -1, -1, -1 }, { -1, -1,  1 },
    { -1,  1, -1 }, { -1,  1,  1 },
    {  1, -1, -1 }, {  1, -1,  1 },
    {  1,  1, -1 }, {  1,  1,  1 }
};

const GLubyte kCubeElements[24] = {
    0, 1, 2, 3, 4, 5, 6, 7,
    0, 2, 1, 3, 4, 6, 5, 7,
    0, 4, 1, 5, 2, 6, 3, 7
};

void Cube::draw()
{
    float d = size_ * 0.5f;
    glPushAttrib(GL_CURRENT_BIT);
    glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
    glPushMatrix();

    glEnableClientState(GL_VERTEX_ARRAY);
    setupMatrix();
    glScalef(d, d, d);
    glTranslatef(0.0f, 0.0f, 1.0f);
    glColor3ubv(color_);
    glVertexPointer(3, GL_SHORT, 0, kCubeVertices);
    glDrawElements(GL_LINES, 24, GL_UNSIGNED_BYTE, kCubeElements);

    glPopMatrix();
    glPopClientAttrib();
    glPopAttrib();
}
