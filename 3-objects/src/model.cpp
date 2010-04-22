#include "model.hpp"
#include "color.hpp"

static const GLfloat kCubeVertices[8][3] = {
    { -1, -1, -1 }, { -1, -1,  1 }, { -1,  1, -1 }, { -1,  1,  1 },
    {  1, -1, -1 }, {  1, -1,  1 }, {  1,  1, -1 }, {  1,  1,  1 }
};

static const GLubyte kCubeTris[12][3] = {
    { 1, 3, 2 }, { 2, 0, 1 },
    { 0, 2, 6 }, { 6, 4, 0 },
    { 4, 6, 7 }, { 7, 5, 4 },
    { 1, 5, 7 }, { 7, 3, 1 },
    { 2, 3, 7 }, { 7, 6, 2 },
    { 0, 4, 5 }, { 5, 1, 0 }
};

static const GLubyte kCubeLines[12][2] = {
    { 0, 1 }, { 2, 3 }, { 4, 5 }, { 6, 7 },
    { 0, 2 }, { 1, 3 }, { 4, 6 }, { 5, 7 },
    { 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 }
};

const Model Model::kCube = {
    8, kCubeVertices, 12, kCubeTris, 12, kCubeLines
};

static const GLfloat kPyramidVertices[8][3] = {
    { -1, -1, -1 }, { -1,  1, -1 }, {  1, -1, -1 }, {  1,  1, -1 },
    {  0,  0,  1 }
};

static const GLubyte kPyramidTris[6][3] = {
    { 0, 1, 3 }, { 3, 2, 0 },
    { 0, 4, 1 }, { 1, 4, 3 },
    { 3, 4, 2 }, { 2, 4, 0 }
};

static const GLubyte kPyramidLines[8][2] = {
    { 0, 1 }, { 2, 3 }, { 0, 2 }, { 1, 3 },
    { 0, 4 }, { 1, 4 }, { 2, 4 }, { 3, 4 }
};

const Model Model::kPyramid = {
    5, kPyramidVertices, 6, kPyramidTris, 8, kPyramidLines
};

void Model::draw(const Color &tcolor, const Color &lcolor) const
{
    glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT);
    glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);

    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.0f, 1.0f);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, vertex);
    glColor3ubv(tcolor.c);
    glDrawElements(GL_TRIANGLES, triCount * 3, GL_UNSIGNED_BYTE, tri);
    glColor3ubv(lcolor.c);
    glDrawElements(GL_LINES, lineCount * 2, GL_UNSIGNED_BYTE, line);

    glPopClientAttrib();
    glPopAttrib();
}
