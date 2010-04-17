#include "world.hpp"
#include "object.hpp"

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
    for (Object *p = first_; p; p = p->next_)
        p->draw();
}

void World::update()
{
    for (Object *p = first_; p; p = p->next_)
        p->update();
}
