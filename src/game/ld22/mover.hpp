#ifndef GAME_LD22_MOVER_HPP
#define GAME_LD22_MOVER_HPP
#include "actor.hpp"
namespace LD22 {

class Mover : public Actor {
public:
    // Multiply all speed by SPEED_SCALE
    static const int SPEED_SCALE = 256;

protected:
    // xspeed, yspeed.  Must be initialized by subclass.
    // These will be changed by Mover::advance if it
    // hits an obstacle.
    int m_xs, m_ys;

public:
    Mover()
        : m_xs(0), m_ys(0)
    { }

    virtual ~Mover();

    virtual void advance();
};

}
#endif
