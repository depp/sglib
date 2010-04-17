#include "world.hpp"
#include "object.hpp"
#include <cmath>
#include "SDL_opengl.h"

const float kPi = 4.0f * std::atan(1.0f);
const float World::kFrameTime = World::kFrameTicks * 0.001f;

World::World()
    : first_(0)
{ }

World::~World()
{
    Object *p = first_, *n;
    while (p) {
        n = p->next_;
        delete p;
        p = n;
    }
}

void World::addObject(Object *obj)
{
    obj->next_ = first_;
    first_ = obj;
}

void World::draw()
{
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    for (Object *p = first_; p; p = p->next_)
        p->draw();
    glDisable(GL_DEPTH_TEST);
}

void World::update()
{
    for (Object *p = first_; p; p = p->next_) {
        float d = kFrameTime * p->speed_;
        float r = p->face_ * (kPi / 180.0f);
        p->x_ += d * std::cos(r);
        p->y_ += d * std::sin(r);
        p->update();
    }
}
