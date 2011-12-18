#include "other.hpp"
#include "item.hpp"
#include "screen.hpp"
#include "area.hpp"
#include "tileset.hpp"
using namespace LD22;

Other::~Other()
{ }

void Other::advance()
{
    scanItems();
    if (m_item) {
        int wx = centerx(), wy = centery();
        int ix = m_item->centerx(), iy = m_item->centery();
        int dx = ix - wx, dy = iy - wy;
        if (dx < -20)
            m_xpush = -PUSH_SCALE;
        else if (dx > 20)
            m_xpush = PUSH_SCALE;
        else
            m_xpush = 0;
        if (dy > 80)
            m_ypush = 1;
        else
            m_ypush = 0;
    } else {
        m_xpush = 0;
        m_ypush = 0;
    }
    Walker::advance();
}

void Other::didFallOut()
{
    Screen &scr = area().screen();
    scr.win();
}

void Other::draw(int delta, Tileset &tiles)
{
    int x, y;
    Walker::draw(delta, tiles);
    getDrawPos(&x, &y, delta);
    tiles.drawWidget(x - 30, y + 40, Widget::ThoughtLeft, 1.0f);
    tiles.drawWidget(x + 15, y + 100, Widget::Star, 0.6f);
}
