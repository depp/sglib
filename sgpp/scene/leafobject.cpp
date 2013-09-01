/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "sgpp/scene/leafobject.hpp"

Scene::LeafObject::~LeafObject()
{ }

void Scene::LeafObject::trace(std::vector<Scene::LeafObject *> &hits,
                              UI::Point pt)
{
    if (hitTest(pt))
        hits.push_back(this);
}
