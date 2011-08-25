#ifndef SCENERY_H
#define SCENERY_H
#include "entity.hpp"
namespace Space {

class Scenery : public Entity {
public:
    Scenery();
    virtual ~Scenery();

    virtual void draw();
};

}
#endif
