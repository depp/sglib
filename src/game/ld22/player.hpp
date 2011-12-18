#ifndef GAME_LD22_PLAYER_HPP
#define GAME_LD22_PLAYER_HPP
#include "walker.hpp"
namespace LD22 {

class Player : public Walker {
public:
    Player(int x, int y)
        : Walker(x, y)
    { }
    virtual ~Player();
    virtual void advance();
};

}
#endif
