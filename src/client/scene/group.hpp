#ifndef SCENE_GROUP_HPP
#define SCENE_GROUP_HPP
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
