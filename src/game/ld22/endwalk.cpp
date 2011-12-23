#include "endwalk.hpp"
using namespace LD22;

EndWalk::~EndWalk()
{ }

void EndWalk::init()
{
    m_timer = 750;
    m_wclass = 2;
}

void EndWalk::advance()
{
    if (!m_exists) {
        m_timer--;
        if (m_timer > 0)
            return;
        m_exists = true;
        m_timer = 40;
    }
    Other::advance();
    m_timer--;
    if (!m_timer) {
        if (m_state == SMunch) {
            setState(SIdle);
            m_timer = 120;
        } else {
            setState(SMunch);
            m_timer = 40;
        }
    }
}

void EndWalk::draw(int delta, Tileset &tiles)
{
    if (m_exists)
        Other::draw(delta, tiles);
}
