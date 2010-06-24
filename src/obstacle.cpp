#include "obstacle.hpp"
#include "model.hpp"

Obstacle::Obstacle(float x, float y, float face, float size,
                   const Model &model, Color tcolor, Color lcolor)
    : Object(kClassSolid, 0, x, y, face, size), model_(model),
      tcolor_(tcolor), lcolor_(lcolor)
{ }

Obstacle::~Obstacle()
{ }

void Obstacle::draw()
{
    glPushMatrix();
    setupMatrix();
    float d = getSize() * 0.5f;
    glScalef(d, d, d);
    glTranslatef(0.0f, 0.0f, 1.0f);
    model_.draw(tcolor_, lcolor_);
    glPopMatrix();
}
