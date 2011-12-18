#ifndef GAME_LD22_PLAYER_HPP
#define GAME_LD22_PLAYER_HPP
#include "walker.hpp"
namespace LD22 {

class Player : public Walker {
public:
    Player(int x, int y)
        : Walker(false)
    {
        m_x = x;
        m_y = y;
    }

    virtual ~Player();
    virtual void advance();
    virtual void didFallOut();
};

}
#endif
