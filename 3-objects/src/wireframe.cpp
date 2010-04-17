#include "wireframe.hpp"
#include "SDL_opengl.h"

struct Wireframe::Model {
    const GLshort (*vertex)[3];
    GLint vertexCount;
    const GLubyte *lineIndex;
    GLint lineIndexCount;
};

static const GLshort kCubeVertices[8][3] = {
    { -1, -1, -1 }, { -1, -1,  1 }, { -1,  1, -1 }, { -1,  1,  1 },
    {  1, -1, -1 }, {  1, -1,  1 }, {  1,  1, -1 }, {  1,  1,  1 }
};

static const GLubyte kCubeElements[24] = {
    0, 1, 2, 3, 4, 5, 6, 7,
    0, 2, 1, 3, 4, 6, 5, 7,
    0, 4, 1, 5, 2, 6, 3, 7
};

const Wireframe::Model Wireframe::kCube = {
    kCubeVertices, 8, kCubeElements, 24
};

static const GLshort kPyramidVertices[8][3] = {
    { -1, -1, -1 }, { -1,  1, -1 }, {  1, -1, -1 }, {  1,  1, -1 },
    {  0,  0,  1 }
};

static const GLubyte kPyramidElements[16] = {
    0, 1, 2, 3, 0, 2, 1, 3, 0, 4, 1, 4, 2, 4, 3, 4
};

const Wireframe::Model Wireframe::kPyramid = {
    kPyramidVertices, 5, kPyramidElements, 16
};

Wireframe::Wireframe(float x, float y, float face, float size,
                     const Model &model, Color color)
    : Object(x, y, face), size_(size), model_(model), color_(color)
{ }

Wireframe::~Wireframe()
{ }

void Wireframe::draw()
{
    glPushAttrib(GL_CURRENT_BIT);
    glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
    glPushMatrix();

    glEnableClientState(GL_VERTEX_ARRAY);
    setupMatrix();
    float d = size_ * 0.5f;
    glScalef(d, d, d);
    glTranslatef(0.0f, 0.0f, 1.0f);
    glColor3ubv(color_.c);
    glVertexPointer(3, GL_SHORT, 0, model_.vertex);
    glDrawElements(GL_LINES, model_.lineIndexCount,
                   GL_UNSIGNED_BYTE, model_.lineIndex);

    glPopMatrix();
    glPopClientAttrib();
    glPopAttrib();
}

void Wireframe::update()
{
    setSpeed(10.0f);
    setFace(getFace() + 1.0f);
}
