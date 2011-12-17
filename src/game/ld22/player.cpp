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
    if (canMove(m_x + dx, m_y + dy)) {
        m_x += dx;
        m_y += dy;
    } else if (canMove(m_x + dx, m_y)) {
        m_x += dx;
    } else if (canMove(m_x, m_y + dy)) {
        m_y += dy;
    }
}
