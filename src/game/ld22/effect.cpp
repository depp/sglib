#include "effect.hpp"
#include "tileset.hpp"
#include "area.hpp"
using namespace LD22;

Effect::~Effect()
{ }

void Effect::draw(int delta, Tileset &tiles)
{
    int x, y;
    int s1, s2;
    getDrawPos(&x, &y, delta);
    switch (m_etype) {
    case ThinkStar:
        s1 = Widget::Star;
        goto thinkOne;

    case ThinkStarBang:
        s1 = Widget::Star;
        s2 = Widget::Bang;
        goto thinkTwo;

    case ThinkStarQuestion:
        s1 = Widget::Star;
        s2 = Widget::Question;
        goto thinkTwo;

    case SayHeart:
        if (!m_right) {
            tiles.drawWidget(x + 20, y + 25, Widget::SpeakLeft, 1.0f);
            tiles.drawWidget(x + 55, y + 65, Widget::Heart, 0.75f);
        } else {
            tiles.drawWidget(x - 120, y + 25, Widget::SpeakRight, 1.0f);
            tiles.drawWidget(x - 80, y + 65, Widget::Heart, 0.75f);
        }
        break;
    }
    return;

thinkOne:
    tiles.drawWidget(x - 30, y + 40, Widget::ThoughtLeft, 1.0f);
    tiles.drawWidget(x + 15, y + 100, s1, 0.8f);
    return;

thinkTwo:
    tiles.drawWidget(x - 30, y + 40, Widget::ThoughtLeft, 1.0f);
    tiles.drawWidget(x + 5, y + 100, s1, 0.6f);
    tiles.drawWidget(x + 35, y + 100, s2, 0.6f);
    return;
}

void Effect::init()
{
    // Destroy any effect on the same actor
    const std::vector<Actor *> &v = area().actors();
    std::vector<Actor *>::const_iterator i = v.begin(), e = v.end();
    for (; i != e; ++i) {
        Actor *a = *i;
        if (a->type() != AEffect || a == this)
            continue;
        Effect &e = *static_cast<Effect *> (a);
        if (e.m_track == m_track) {
            e.destroy();
            break;
        }
    }

    switch (m_etype) {
    case ThinkStar:         m_timer = 30; break;
    case ThinkStarBang:     m_timer = 20; break;
    case ThinkStarQuestion: m_timer = 20; break;
    case SayHeart:          m_timer = 30; break;
    }
    // Only effects actually flip
    m_right = m_x >= SCREEN_WIDTH / 2;
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

