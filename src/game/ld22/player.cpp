#include "player.hpp"
#include "screen.hpp"
#include "defs.hpp"
#include "area.hpp"
using namespace LD22;

Player::~Player()
{ }

void Player::advance()
{
    Screen &scr = m_area->screen();
    int dx = 0, dy = 0;
    if (scr.getKey(InRight))
        dx += 1;
    if (scr.getKey(InLeft))
        dx -= 1;
    if (scr.getKey(InUp))
        dy += 1;
    /*
    if (scr.getKey(InDown))
        dy -= 1;
    */
    m_xpush = dx * 4;
    m_ypush = dy * 8;
    Walker::advance();
}
