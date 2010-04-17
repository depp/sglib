#ifndef WIREFRAME_HPP
#define WIREFRAME_HPP
#include "object.hpp"
#include "color.hpp"

class Wireframe : public Object {
public:
    struct Model;
    static const Model kPyramid, kCube;

    Wireframe(float x, float y, float face, float size,
              const Model &model, Color color);
    virtual ~Wireframe();
    virtual void draw();
    virtual void update();

private:
    float size_;
    const Model &model_;
    Color color_;
};

#endif
