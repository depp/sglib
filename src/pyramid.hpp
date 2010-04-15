#ifndef PYRAMID_HPP
#define PYRAMID_HPP
#include "object.hpp"

class Pyramid : public Object {
public:
    Pyramid(float x, float y, float size, const unsigned char color[3]);
    virtual ~Pyramid();
    virtual void draw();

private:
    float size_;
    unsigned char color_[3];
};

#endif
