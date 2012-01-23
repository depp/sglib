#include "actor.hpp"
#include "area.hpp"
#include "defs.hpp"
#include "client/opengl.hpp"
#include "client/rand.hpp"
using namespace LD22;

Actor::~Actor()
{ }

void Actor::wasDestroyed()
{ }

void Actor::draw(int delta, Tileset &tiles)
{
    (void) &tiles;
    drawHitBox(delta);
}

void Actor::drawHitBox(int delta)
{
    int x, y;
    getDrawPos(&x, &y, delta);
    int w = m_w, h = m_h;
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4ub(255, 40, 40, 128);
    glBegin(GL_QUADS);
    glVertex2s(x, y);
    glVertex2s(x, y + h);
    glVertex2s(x + w, y + h);
    glVertex2s(x + w, y);
    glEnd();
    glColor3ub(255, 255, 255);
    glDisable(GL_BLEND);
}

void Actor::advance()
{ }

void Actor::init()
{ }

bool Actor::wallAt(int x, int y)
{
    Area &a = *m_area;
    int s = TILE_BITS, w = m_w, h = m_h;
    for (int yp = y >> s; (yp << s) < y + h; ++yp) {
        for (int xp = x >> s; (xp << s) < x + w; ++xp) {
            int t = a.getTile(xp, yp);
            if (t)
                return true;
        }
    }
    return false;
}

int Actor::moveTo(int x, int y)
{
    if (!wallAt(x, y)) {
        m_x = x;
        m_y = y;
        return MoveFull;
    } else {
        int x0 = m_x, y0 = m_y;
        int dx = x - x0, dy = y - y0;
        unsigned adx = dx > 0 ? dx : -dx;
        unsigned ady = dy > 0 ? dy : -dy;
        if (adx <= 1 && ady <= 1)
            return MoveNone;
        int r = moveTo(x0 + dx/2, y0 + dy/2);
        if (r == MoveFull)
            r = moveTo(x, y);
        return r;
    }
}

// +/- 32 to visibility test endpoint coordinates
static int LOOK_OFFSET = 64;

bool Actor::visible(Actor *a)
{
    if (!a || !a->isvalid())
        return false;
    int c[4], i;
    c[0] = centerx(); c[1] = centery();
    c[2] = a->centerx(); c[3] = a->centery();
    unsigned r = Rand::girand();
    for (i = 0; i < 4; ++i)
        c[i] += (int)((r >> (8*i)) & LOOK_OFFSET) - LOOK_OFFSET / 2;
    return m_area->trace(c[0], c[1], c[2], c[3]);
}
