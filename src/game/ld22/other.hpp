#ifndef GAME_LD22_OTHER_HPP
#define GAME_LD22_OTHER_HPP
#include "walker.hpp"
namespace LD22 {

class Other : public Walker {
    typedef enum {
        SIdle,
        SAha,
        SChase
    } State;

    State m_state;
    int m_timer;

public:
    Other(int x, int y)
        : m_state(SIdle)
    {
        m_x = x;
        m_y = y;
    }

    virtual ~Other();
    virtual void advance();
    virtual void didFallOut();
    virtual void draw(int delta, Tileset &tiles);

private:
    void idle();
    void chase();
    void aha();
    void setState(State s);
};

}
#endif
