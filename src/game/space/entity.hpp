#ifndef GAME_SPACE_ENTITY_HPP
#define GAME_SPACE_ENTITY_HPP
#include "vector.hpp"
namespace Space {
class World;

class Entity {
public:
    Entity();
    virtual ~Entity();

    vector location;
    float radius;
    int layer;

    virtual void move(World &w, double delta);
    virtual void draw() = 0;
};

}
#endif
