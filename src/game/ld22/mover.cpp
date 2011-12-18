#include "mover.hpp"
using namespace LD22;

const static int MAX_SPEED = SPEED_SCALE * 32;

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
    if (m_bounded) {
        if (nx < 0) {
            nx = 0;
            m_xs = 0;
        } else if (nx > SCREEN_WIDTH - m_w) {
            nx = SCREEN_WIDTH - m_w;
            m_xs = 0;
        }
        if (ny < 0) {
            ny = 0;
            m_ys = 0;
        } else if (ny > SCREEN_HEIGHT - m_h) {
            ny = SCREEN_HEIGHT - m_h;
            m_ys = 0;
        }
    }
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
