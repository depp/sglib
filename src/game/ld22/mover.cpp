#include "mover.hpp"
using namespace LD22;

const static int MAX_SPEED = Mover::SPEED_SCALE * 64;

Mover::~Mover()
{ }

static void clip(int &x)
{
    if (x < -MAX_SPEED)
        x = -MAX_SPEED;
    else if (x > MAX_SPEED)
        x = MAX_SPEED;
}

void Mover::advance()
{
    clip(m_xs);
    clip(m_ys);
    int dx = m_xs / SPEED_SCALE, dy = m_ys / SPEED_SCALE;
    if (!dx && !dy)
        return;
    int nx = m_x + dx, ny = m_y + dy;
    if (moveTo(nx, ny) != MoveFull) {
        int r;
        if (dy) {
            r = moveTo(m_x, ny);
            if (r != MoveNone) {
                m_xs = 0;
                if (r == MovePartial)
                    m_ys = 0;
                return;
            }
        }
        if (dy) {
            r = moveTo(nx, m_y);
            if (r != MoveNone) {
                m_ys = 0;
                if (r == MovePartial)
                    m_xs = 0;
                return;
            }
        }
        m_xs = 0;
        m_ys = 0;
    }
}
