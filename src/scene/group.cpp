#include "group.hpp"

Scene::Group::~Group()
{ }

void Scene::Group::trace(std::vector<Scene::LeafObject *> &hits, UI::Point pt)
{
    std::vector<Object *>::iterator
        i = objects_.begin(), e = objects_.end();
    for (; i != e; ++i)
        (*i)->trace(hits, pt);
}

void Scene::Group::draw(unsigned int ticks)
{
    std::vector<Object *>::iterator
        i = objects_.begin(), e = objects_.end();
    for (; i != e; ++i)
        (*i)->draw(ticks);
}

void Scene::Group::addObject(Scene::Object *obj)
{
    objects_.push_back(obj);
}
