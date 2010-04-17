#ifndef SHOT_HPP
#define SHOT_HPP
#include "object.hpp"

class Shot : public Object {
public:
    Shot(float x, float y, float face);
    virtual ~Shot();
    virtual void draw();
    virtual void update();
};

#endif
