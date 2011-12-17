#include "walker.hpp"
#include "client/texturefile.hpp"
#include <stdio.h>
using namespace LD22;

Walker::~Walker()
{ }

void Walker::draw(int delta)
{
    int x, y;
    getDrawPos(&x, &y, delta);
    float x0 = x + WWIDTH/2 - 32, x1 = x0 + 64;
    float y0 = y + WHEIGHT/2 - 32, y1 = y0 + 64;
    float u0 = 0.0f, u1 = 0.125f, v0 = 1.0f, v1 = 0.75f;
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_DST_COLOR, GL_ZERO);
    m_tex->bind();
    glBegin(GL_QUADS);
    glTexCoord2f(u0, v0); glVertex2f(x0, y0);
    glTexCoord2f(u0, v1); glVertex2f(x0, y1);
    glTexCoord2f(u1, v1); glVertex2f(x1, y1);
    glTexCoord2f(u1, v0); glVertex2f(x1, y0);
    glEnd();
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);

    drawHitBox(delta);
}

void Walker::init()
{
    m_xspeed = 0;
    m_yspeed = 0;
    m_xpush = 0;
    checkState();
    m_tex = TextureFile::open("sprite/stick.png");
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
