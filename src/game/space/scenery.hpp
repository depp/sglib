#ifndef GAME_SPACE_SCENERY_HPP
#define GAME_SPACE_SCENERY_HPP
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
