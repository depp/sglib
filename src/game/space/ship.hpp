#ifndef GAME_SPACE_SHIP_HPP
#define GAME_SPACE_SHIP_HPP
#include "entity.hpp"
namespace Space {

class Ship : public Entity {
public:
    Ship();
    virtual ~Ship();

    virtual void move(World &g, double delta);
    virtual void draw();

    float angle;
    float angular_velocity;
    vector direction;
    vector velocity;
};

}
#endif
