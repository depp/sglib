#include "other.hpp"
#include "item.hpp"
#include "screen.hpp"
#include "area.hpp"
#include "tileset.hpp"
#include "sys/rand.hpp"
using namespace LD22;

static int lookInterval()
{
    static int LOOK_INTERVAL = 15;
    return LOOK_INTERVAL + (Rand::girand() % LOOK_INTERVAL);
}

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

    case SMunch:
        munch();
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
    if (m_timer >= 0) {
        scanItems();
        if (m_item && visible(m_item)) {
            setState(SAha);
            return;
        }
        m_timer = -lookInterval();
    }
}

void Other::chase()
{
    if (!m_item) {
        setState(SIdle);
        return;
    }
    if (m_timer >= 0) {
        if (!visible(m_item)) {
            setState(SIdle);
            return;
        }
        m_timer = -lookInterval();
    }
    int wx = centerx(), wy = centery();
    int ix = m_item->centerx(), iy = m_item->centery();
    int dx = ix - wx, dy = iy - wy;

    bool inReach = false;
    if (m_item->m_owner) {
        if (m_item_distance < PICKUP_DISTANCE * 3/2)
            inReach = true;
    } else {
        if (m_item_distance < PICKUP_DISTANCE)
            inReach = true;
    }
    if (inReach && pickupItem()) {
        m_item->m_locked = true;
        setState(SMunch);
        m_xpush = 0;
        m_ypush = 0;
        return;
    }

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

void Other::munch()
{
    
}

void Other::setState(State s)
{
    m_state = s;
    m_timer = 0;
}
