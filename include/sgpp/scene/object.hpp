#ifndef CLIENT_SCENE_OBJECT_HPP
#define CLIENT_SCENE_OBJECT_HPP
#include <vector>
#include "client/ui/geometry.hpp"
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
