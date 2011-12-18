#include "other.hpp"
#include "item.hpp"
#include "screen.hpp"
#include "area.hpp"
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
