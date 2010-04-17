#include "object.hpp"
#include "SDL_opengl.h"

Object::Object(float x, float y, float face)
    : x_(x), y_(y), face_(face), next_(0), prev_(0)
{ }

Object::~Object()
{ }

void Object::setupMatrix()
{
    glTranslatef(x_, y_, 0.0f);
    glRotatef(face_, 0.0f, 0.0f, 1.0f);
}

void Object::update()
{ }
