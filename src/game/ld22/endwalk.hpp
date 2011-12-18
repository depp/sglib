#ifndef GAME_LD22_ENDWALK_HPP
#define GAME_LD22_ENDWALK_HPP
#include "other.hpp"
namespace LD22 {

class EndWalk : public Other {
public:
    bool m_exists;
    int m_timer;

    EndWalk(int x, int y)
        : Other(x, y)
    {
        m_bounded = true;
    }

    virtual ~EndWalk();
    virtual void init();
    virtual void advance();
    virtual void draw(int delta, Tileset &tiles);

};

}
#endif
