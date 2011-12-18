#include "other.hpp"
#include "item.hpp"
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
        (void) dy;
    } else {
        m_xpush = 0;
    }
    Walker::advance();
}
