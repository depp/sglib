#ifndef GRAPHICS_MODEL_HPP
#define GRAPHICS_MODEL_HPP
#include "SDL_opengl.h"
struct Color;

struct Model {
    static const Model kCube, kPyramid;

    void draw(const Color tcolor, const Color lcolor) const;

    GLint vertexCount;
    const GLfloat (*vertex)[3];
    GLsizei triCount;
    const GLubyte (*tri)[3];
    GLsizei lineCount;
    const GLubyte (*line)[2];
};

#endif
