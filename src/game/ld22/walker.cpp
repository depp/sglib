#include "walker.hpp"
#include <stdio.h>
using namespace LD22;

Walker::~Walker()
{ }

void Walker::init()
{
    m_xspeed = 0;
    m_yspeed = 0;
    m_xpush = 0;
    checkState();
}

#if 0
static int clip(int x, int n)
{
    if (x > n)
        return n;
    if (x < -n)
        return -n;
    return x;
}
#endif

static int advanceSpeed(int speed, int nspeed, int traction)
{
    int d = nspeed - speed;
    if (d < -traction)
        d = -traction;
    else if (d > traction)
        d = traction;
    return speed + d;
}

void Walker::advance()
{
    int traction = 0;
    checkState();

    switch (m_wstate) {
    case WStand:
        traction = SPEED_SCALE;
        m_yspeed = m_ypush * SPEED_SCALE;
        break;

    case WFall:
        traction = SPEED_SCALE / 4;
        m_yspeed -= GRAVITY;
        break;
    }
    m_xspeed = advanceSpeed(m_xspeed, m_xpush * SPEED_SCALE, traction);

    moveTo(m_x + m_xspeed / SPEED_SCALE,
           m_y + m_yspeed / SPEED_SCALE);
}

void Walker::checkState()
{
    if (m_yspeed <= 0 && wallAt(m_x, m_y - 1)) {
        m_wstate = WStand;
        m_yspeed = 0;
    } else {
        m_wstate = WFall;
    }
}
