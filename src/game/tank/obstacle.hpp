#ifndef GAME_TANK_OBSTACLE_HPP
#define GAME_TANK_OBSTACLE_HPP
#include "object.hpp"
#include "graphics/color.hpp"
#include "graphics/model.hpp"
namespace Tank {

class Obstacle : public Object {
public:
    Obstacle(float x, float y, float face, float size,
             Model::Ref model, Color tcolor, Color lcolor);
    virtual ~Obstacle();
    virtual void draw();

private:
    Model::Ref model_;
    Color tcolor_, lcolor_;
};

}
#endif
