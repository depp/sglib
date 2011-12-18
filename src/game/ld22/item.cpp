#include "item.hpp"
#include "tileset.hpp"
#include "walker.hpp"
using namespace LD22;

Item::~Item()
{ }

void Item::draw(int delta, Tileset &tiles)
{
    int x, y;
    getDrawPos(&x, &y, delta);
    tiles.drawWidget(x, y, Widget::Star);
}


void Item::advance()
{
    if (m_state == SFree) {
        Mover::advance();
    } else {
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
