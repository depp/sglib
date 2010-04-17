#include "object.hpp"
#include "SDL_opengl.h"

Object::Object(float x, float y)
    : x_(x), y_(y), next_(0), prev_(0)
{ }

Object::~Object()
{ }

void Object::setupMatrix()
{
    glTranslatef(x_, y_, 0.0f);
}

void Object::update()
{ }
