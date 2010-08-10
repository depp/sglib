#ifndef GAME_OBSTACLE_HPP
#define GAME_OBSTACLE_HPP
#include "object.hpp"
#include "color.hpp"
struct Model;

class Obstacle : public Object {
public:
    Obstacle(float x, float y, float face, float size,
             const Model &model, Color tcolor, Color lcolor);
    virtual ~Obstacle();
    virtual void draw();

private:
    const Model &model_;
    Color tcolor_, lcolor_;
};

#endif
