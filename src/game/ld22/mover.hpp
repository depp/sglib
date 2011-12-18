#ifndef GAME_LD22_MOVER_HPP
#define GAME_LD22_MOVER_HPP
#include "actor.hpp"
namespace LD22 {

class Mover : public Actor {
protected:
    // xspeed, yspeed.  Must be initialized by subclass.
    // These will be changed by Mover::advance if it
    // hits an obstacle.
    // Multiply by SPEED_SCALE.
    int m_xs, m_ys;

public:
    explicit Mover(Type t)
        : Actor(t), m_xs(0), m_ys(0)
    { }

    virtual ~Mover();

    virtual void advance();
};

}
#endif
