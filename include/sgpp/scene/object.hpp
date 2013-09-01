/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#ifndef SGPP_SCENE_OBJECT_HPP
#define SGPP_SCENE_OBJECT_HPP
#include <vector>
#include "sgpp/ui/geometry.hpp"
namespace Scene {
class LeafObject;

class Object {
public:
    virtual ~Object();
    virtual void trace(std::vector<LeafObject *> &hits, UI::Point pt) = 0;
    virtual void draw() = 0;
};

}
#endif
