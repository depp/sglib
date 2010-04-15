#ifndef PYRAMID_HPP
#define PYRAMID_HPP
#include "object.hpp"
#include "color.hpp"

class Pyramid : public Object {
public:
    Pyramid(float x, float y, float size, Color color);
    virtual ~Pyramid();
    virtual void draw();

private:
    float size_;
    Color color_;
};

#endif
