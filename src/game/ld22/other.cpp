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
    m_timer += 1;
    m_xpush = 0;
    m_ypush = 0;
    updateItem();

    switch (m_state) {
    case SChase:
        chase();
        break;

    case SIdle:
        idle();
        break;

    case SAha:
        aha();
        break;
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

    switch (m_state) {
    default:
        break;

    case SAha:
        if (m_timer > 15) {
            tiles.drawWidget(x - 30, y + 40, Widget::ThoughtLeft, 1.0f);
            tiles.drawWidget(x + 15, y + 100, Widget::Star, 0.6f);
        }
        break;
    }
}

void Other::idle()
{
    scanItems();
    if (m_item) {
        setState(SAha);
        return;
    }
}

void Other::chase()
{
    if (!m_item) {
        setState(SIdle);
        return;
    }
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
}

void Other::aha()
{
    if (!m_item) {
        setState(SIdle);
        return;
    }
    if (m_timer == 30) {
        setState(SChase);
        return;
    }
}

void Other::setState(State s)
{
    m_state = s;
    m_timer = 0;
}
