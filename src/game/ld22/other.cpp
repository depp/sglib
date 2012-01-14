#include "other.hpp"
#include "item.hpp"
#include "area.hpp"
#include "tileset.hpp"
#include "effect.hpp"
#include "client/rand.hpp"
#include <stdlib.h>
using namespace LD22;

static int AHA_TIME = 15;

static int lookInterval()
{
    static int LOOK_INTERVAL = 15;
    return LOOK_INTERVAL + (Rand::girand() % LOOK_INTERVAL);
}

Other::~Other()
{ }

void Other::wasDestroyed()
{
    area().removeOther();
}

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

void Other::idle()
{
    if (m_timer >= 0) {
        m_visfail = 0;
        scanItems();
        if (itemVisible()) {
            setState(SAha);
            area().addActor(new Effect(Effect::ThinkStar, this));
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
        if (!itemVisible()) {
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
        area().addActor(new Effect(Effect::SayHeart, this));
        m_item->markAsLost();
        return;
    }

    if (dx < -20)
        m_xpush = -PUSH_SCALE;
    else if (dx > 20)
        m_xpush = PUSH_SCALE;
    else
        m_xpush = 0;
    m_ypush = 0;
    if (abs(dx) < 150) {
        if (dy > 80)
            m_ypush = PUSH_SCALE;
        else if (dy > 20)
            m_ypush = PUSH_SCALE / 2;
    }
}

void Other::aha()
{
    if (!m_item) {
        setState(SIdle);
        return;
    }
    if (m_timer == AHA_TIME) {
        setState(SChase);
        return;
    }
}

void Other::munch()
{
    
}

void Other::setState(State s)
{
    if (0) {
        switch (s) {
        case SIdle: puts("idle"); break;
        case SAha: puts("aha"); break;
        case SChase: puts("chase"); break;
        case SMunch: puts("munch"); break;
        }
    }
    m_state = s;
    m_timer = 0;
}

// This smooths over the results and changes a few "false" results to
// "true".
bool Other::itemVisible()
{
    if (!m_item) {
        m_visfail = 0;
        return false;
    }
    if (visible(m_item))
        goto yes;
    if (m_item->m_state == Item::SGrabbed && visible(m_item->m_owner))
        goto yes;
    if (m_visfail) {
        m_visfail--;
        return true;
    } else {
        return false;
    }

yes:
    m_visfail = 1;
    return true;
}
