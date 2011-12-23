#ifndef GAME_LD22_OTHER_HPP
#define GAME_LD22_OTHER_HPP
#include "walker.hpp"
namespace LD22 {

class Other : public Walker {
protected:
    typedef enum {
        SIdle,
        SAha,
        SChase,
        SMunch
    } State;

    State m_state;
    int m_timer;
    int m_visfail;

public:
    Other(int x, int y)
        : Walker(1), m_state(SIdle), m_timer(0), m_visfail(0)
    {
        m_x = x;
        m_y = y;
    }

    virtual ~Other();
    virtual void advance();
    virtual void wasDestroyed();

protected:
    void idle();
    void chase();
    void aha();
    void munch();
    void setState(State s);
    bool itemVisible();
};

}
#endif
