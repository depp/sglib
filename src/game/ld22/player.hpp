#ifndef GAME_LD22_PLAYER_HPP
#define GAME_LD22_PLAYER_HPP
#include "walker.hpp"
namespace LD22 {
class Screen;

class Player : public Walker {
public:
    Player(int x, int y, Screen &scr)
        : Walker(x, y), m_scr(scr)
    { }
    virtual ~Player();
    virtual void advance();

private:
    Screen &m_scr;
};

}
#endif
