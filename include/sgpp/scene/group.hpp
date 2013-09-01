/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#ifndef SGPP_SCENE_GROUP_HPP
#define SGPP_SCENE_GROUP_HPP
#include "object.hpp"
namespace Scene {

class Group : public Object {
public:
    virtual ~Group();
    virtual void trace(std::vector<LeafObject *> &hits, UI::Point pt);
    virtual void draw();
    void addObject(Object *obj);

private:
    std::vector<Object *> objects_;
};

}
#endif
