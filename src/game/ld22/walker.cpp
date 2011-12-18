#include "area.hpp"
#include "walker.hpp"
#include "tileset.hpp"
#include "client/texturefile.hpp"
#include "sys/rand.hpp"
#include <stdio.h>
using namespace LD22;

Walker::~Walker()
{ }

void Walker::draw(int delta, Tileset &tiles)
{
    int x, y;
    getDrawPos(&x, &y, delta);
    tiles.drawStick(x, y, m_sprite);
}

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
    static const int MIN_WALK = SPEED_SCALE * 2;
    static const int MIN_FALL = SPEED_SCALE * 2;

    int traction = 0;
    WState prevstate = m_wstate;
    int prevyspeed = m_yspeed;
    WAnim newanim = AStand;
    int atime;
    checkState();

    switch (m_wstate) {
    case WStand:
        traction = SPEED_SCALE;
        m_yspeed = m_ypush * SPEED_SCALE;
        if (!m_animlock) {
            if (prevstate == WFall && prevyspeed < -MIN_FALL)
                newanim = m_xspeed < 0 ? ALandLeft : ALandRight;
            else if (m_xspeed < -MIN_WALK)
                newanim = AWalkLeft;
            else if (m_xspeed > MIN_WALK)
                newanim = AWalkRight;
            else
                newanim = AStand;
        } else {
            newanim = m_anim;
        }
        break;

    case WFall:
        m_animlock = false;
        traction = SPEED_SCALE / 4;
        m_yspeed -= GRAVITY;
        newanim = AFall;
        break;
    }
    m_xspeed = advanceSpeed(m_xspeed, m_xpush * SPEED_SCALE, traction);

    if (newanim == m_anim)
        atime = --m_animtime;
    else
        atime = -1;
    unsigned x;
    switch (newanim) {
    case AStand:
        if (atime <= 0) {
            x = Rand::girand() % 6;
            m_sprite = 0 + (x >= 3 ? x - 3 + 0x80 : x);
            m_animtime = 20 + (Rand::girand() % 40);
        }
        break;

    case AFall:
        if (atime <= 0) {
            x = Rand::girand() % 16;
            m_sprite = 16 + (x >= 8 ? x - 8 + 0x80 : x);
            m_animtime = m_yspeed > 0 ? 10 : 2;
        }
        break;

    case ALandLeft:
    case ALandRight:
        if (atime == 0) {
            m_sprite++;
            if ((m_sprite & 0x7f) == 15)
                m_animlock = false;
            m_animtime = 2;
        } else if (atime < 0) {
            m_sprite = 10 + (newanim == ALandLeft ? 0x80 : 0);
            m_animtime = 2;
            m_animlock = true;
        }
        break;

    case AWalkLeft:
    case AWalkRight:
        if (atime == 0) {
            m_sprite++;
            if ((m_sprite & 0x7f) == 32)
                m_sprite -= 8;
            m_animtime = 2;
        } else if (atime < 0) {
            m_sprite = 24 + (newanim == AWalkLeft ? 0x80 : 0);
            m_animtime = 2;
        }
        break;

    default:
        m_sprite = 0;
        break;
    }
    m_anim = newanim;

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
