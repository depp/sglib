#ifndef SCENE_OBJECT_HPP
#define SCENE_OBJECT_HPP
#include <vector>
#include "ui/geometry.hpp"
namespace Scene {
class LeafObject;

class Object {
public:
    virtual ~Object();
    virtual void trace(std::vector<LeafObject *> &hits, UI::Point pt) = 0;
    virtual void draw(unsigned int ticks) = 0;
};

}
#endif
