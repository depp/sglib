/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#ifndef SGPP_SCENE_LEAFOBJECT_HPP
#define SGPP_SCENE_LEAFOBJECT_HPP
#include "object.hpp"
namespace Scene {

class LeafObject : public Object {
public:
    virtual ~LeafObject();
    virtual void trace(std::vector<LeafObject *> &hits, UI::Point pt);
    virtual bool hitTest(UI::Point pt) = 0;
};

}
#endif
