#ifndef CUBE_HPP
#define CUBE_HPP
#include "object.hpp"

class Cube : public Object {
public:
    Cube(float x, float y, float size, const unsigned char color[3]);
    virtual ~Cube();
    virtual void draw();

private:
    float size_;
    unsigned char color_[3];
};

#endif
