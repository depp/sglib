#include "player.hpp"
#include "screen.hpp"
#include "defs.hpp"
#include "area.hpp"
#include "item.hpp"
using namespace LD22;

Player::~Player()
{ }

void Player::advance()
{
    Screen &scr = area().screen();
    int dx = 0, dy = 0;
    if (scr.getKey(InRight))
        dx += 1;
    if (scr.getKey(InLeft))
        dx -= 1;
    if (scr.getKey(InUp))
        dy += 1;
    if (scr.getKey(InThrow) && haveGrab()) {
        dropItem();
        m_item->m_xs = m_xs * 2;
        m_item->m_ys = m_ys + SPEED_SCALE * 8;
        // Don't pick it back up immediately
        m_pickuptimer = 32;
    }
    /*
    if (scr.getKey(InDown))
        dy -= 1;
    */
    m_xpush = dx * PUSH_SCALE;
    m_ypush = dy * PUSH_SCALE;
    scanItems();
    if (m_pickuptimer) {
        m_pickuptimer--;
    } else {
        if (m_item && m_item_distance < PICKUP_DISTANCE)
            pickupItem();
    }
    Walker::advance();
}

void Player::didFallOut()
{
    Screen &scr = area().screen();
    scr.lose();
}
