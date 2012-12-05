/* Copyright 2012 Dietrich Epp <depp@zdome.net> */
#include "sgpp/scene/leafobject.hpp"

Scene::LeafObject::~LeafObject()
{ }

void Scene::LeafObject::trace(std::vector<Scene::LeafObject *> &hits,
                              UI::Point pt)
{
    if (hitTest(pt))
        hits.push_back(this);
}
