#ifndef GAME_LD22_PLAYER_HPP
#define GAME_LD22_PLAYER_HPP
#include "actor.hpp"
namespace LD22 {
class Screen;

class Player : public Actor {
public:
    Player(int x, int y, Screen &scr)
        : Actor(x, y, 24, 32), m_scr(scr)
    { }
    virtual ~Player();
    virtual void advance();

private:
    Screen &m_scr;
};

}
#endif
