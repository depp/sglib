#include "player.hpp"
#include "screen.hpp"
#include "defs.hpp"
using namespace LD22;

Player::~Player()
{ }

void Player::advance()
{
    int dx = 0, dy = 0;
    if (m_scr.getKey(InRight))
        dx += 1;
    if (m_scr.getKey(InLeft))
        dx -= 1;
    if (m_scr.getKey(InUp))
        dy += 1;
    if (m_scr.getKey(InDown))
        dy -= 1;
    int speed = 4;
    moveTo(m_x + dx * speed, m_y + dy * speed);
}
