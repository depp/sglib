#ifndef CUBE_HPP
#define CUBE_HPP
#include "object.hpp"
#include "color.hpp"

class Cube : public Object {
public:
    Cube(float x, float y, float size, Color color);
    virtual ~Cube();
    virtual void draw();

private:
    float size_;
    Color color_;
};

#endif
