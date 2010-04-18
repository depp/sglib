#include "object.hpp"
#include "SDL_opengl.h"

Object::Object(float x, float y, float face, float size)
    : x_(x), y_(y), face_(face), size_(size), speed_(0.0f),
      index_(0), world_(0)
{ }

Object::~Object()
{ }

void Object::init()
{ }

void Object::draw()
{ }

void Object::setupMatrix()
{
    glTranslatef(x_, y_, 0.0f);
    glRotatef(face_, 0.0f, 0.0f, 1.0f);
}

void Object::update()
{ }
