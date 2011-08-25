#ifndef GAME_TANK_EXPLOSION_HPP
#define GAME_TANK_EXPLOSION_HPP
#include "object.hpp"
namespace Tank {

class Explosion : public Object {
public:
    struct Type;
    static const Type kShot;

    Explosion(float x, float y, const Type &type);
    virtual ~Explosion();
    virtual void init();
    virtual void draw();
    virtual void update();

private:
    struct Particle;

    Explosion(const Explosion &);
    Explosion &operator=(const Explosion &);

    const Type &type_;
    float time_;
    Particle *particle_;
};

}
#endif
