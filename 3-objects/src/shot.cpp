#include "shot.hpp"
#include "model.hpp"
#include "color.hpp"
#include "SDL_opengl.h"

Shot::Shot(float x, float y, float face)
    : Object(x, y, face, 0.25)
{
    setSpeed(25.0f);
}

Shot::~Shot()
{ }

void Shot::draw()
{
    glPushMatrix();
    setupMatrix();
    glTranslatef(0.0f, 0.0f, 0.5f);
    glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
    glScalef(0.125f, 0.125f, 0.5f);
    Model::kPyramid.draw(Color::green, Color::lime);
    glPopMatrix();
}

void Shot::update()
{ }
