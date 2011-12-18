#include "item.hpp"
#include "tileset.hpp"
#include "walker.hpp"
#include <stdio.h>
using namespace LD22;

Item::~Item()
{ }

void Item::draw(int delta, Tileset &tiles)
{
    int x, y;
    getDrawPos(&x, &y, delta);

    switch (m_itype) {
    case Star:
        tiles.drawWidget(x, y, Widget::Star, 1.0f);
        break;

    case Bomb:
        tiles.drawWidget(x, y, Widget::Bomb, 1.0f);
        break;
    }
}

void Item::advance()
{
    if (m_state == SFree) {
    free:
        m_ys -= GRAVITY;
        Mover::advance();
        if (m_y < -100)
            destroy();
    } else {
        if (!m_owner->isvalid() || m_owner->m_item != this) {
            m_locked = false;
            m_state = SFree;
            goto free;
        }
        m_xs = 0;
        m_ys = 0;
        int div, lx = m_x, ly = m_y, tx, ty, nx, ny, dx, dy;
        m_owner->getGrabPos(&tx, &ty);
        tx -= IWIDTH / 2;
        ty -= IWIDTH / 2;
        switch (m_state) {
        case SGrabbing:
            div = 10 - ++m_frame;
            if (div <= 1) {
                m_frame = 0;
                m_state = SGrabbed;
                goto grabbed;
            }
            dx = (tx - lx) / div;
            dy = (ty - ly) / div;
            nx = lx + dx;
            ny = ly + dy;
            break;

        grabbed:
        case SGrabbed:
            dx = tx - lx;
            dy = ty - ly;
            nx = tx;
            ny = ty;
            break;

        default:
            return;
        }
        m_x = nx;
        m_y = ny;
        m_xs = dx * SPEED_SCALE;
        m_ys = dy * SPEED_SCALE;
    }
}

static signed char DIRECTION[8][2] = {
    { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 },
    { 1, 1 }, { -1, -1 }, { -1, 1 }, { 1, -1 }
};

void Item::setFree()
{
    m_owner = NULL;
    m_state = Item::SFree;
    m_locked = false;
    if (!wallAt(m_x, m_y))
        return;
    // It's wedged!  Unwedge it...
    puts("Wedged throw...");
    // 40 pixels in any direction, increments of 8
    int x = m_x, y = m_y;
    for (int i = 1; i <= 5; ++i) {
        int d = i * 8;
        for (int j = 0; j < 8; ++j) {
            int nx = x + DIRECTION[j][0] * d;
            int ny = y + DIRECTION[j][1] * d;
            if (!wallAt(nx, ny)) {
                m_x = nx;
                m_y = ny;
                printf("Wedge resolved with move of %d\n", d);
                return;
            }
        }
    }
    puts("Wedge unresolved!");
}
