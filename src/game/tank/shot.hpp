#ifndef GAME_SHOT_HPP
#define GAME_SHOT_HPP
#include "object.hpp"
namespace Tank {

class Shot : public Object {
public:
    Shot(float x, float y, float face);
    virtual ~Shot();
    virtual void init();
    virtual void draw();
    virtual void update();
    virtual bool collide(Object &other);

private:
    float time_;
};

}
#endif
