#include "effect.hpp"
#include "tileset.hpp"
using namespace LD22;

Effect::~Effect()
{ }

void Effect::draw(int delta, Tileset &tiles)
{
    int x, y;
    getDrawPos(&x, &y, delta);
    switch (m_etype) {
    case ThinkStar:
        tiles.drawWidget(x - 30, y + 40, Widget::ThoughtLeft, 1.0f);
        tiles.drawWidget(x + 15, y + 100, Widget::Star, 0.6f);
        break;

    case ThinkStarBang:
        break;

    case ThinkStarQuestion:
        break;

    case SayHeart:
        break;
    }
}

void Effect::init()
{
    switch (m_etype) {
    case ThinkStar:         m_timer = 30; break;
    case ThinkStarBang:     m_timer = 20; break;
    case ThinkStarQuestion: m_timer = 20; break;
    case SayHeart:          m_timer = 30; break;
    }
}

void Effect::advance()
{
    m_timer--;
    if (m_timer == 0) {
        destroy();
        return;
    }
    if (m_track) {
        if (m_track->isvalid()) {
            m_x = m_track->m_x;
            m_y = m_track->m_y;
        } else {
            m_track = NULL;
        }
    }
}

