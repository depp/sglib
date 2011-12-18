#ifndef GAME_LD22_EFFECT_HPP
#define GAME_LD22_EFFECT_HPP
#include "actor.hpp"
namespace LD22 {

class Effect : public Actor {
public:
    typedef enum {
        ThinkStar,
        ThinkStarBang,
        ThinkStarQuestion,
        SayHeart
    } EType;

private:
    EType m_etype;
    Actor *m_track;
    int m_timer;

public:

    Effect(EType t, int x, int y)
        : Actor(AEffect), m_etype(t), m_track(0), m_timer(0)
    {
        m_x = x;
        m_y = y;
    }

    Effect(EType t, Actor *track)
        : Actor(AEffect), m_etype(t), m_track(track)
    {
        m_x = track->m_x;
        m_y = track->m_y;
    }

    virtual ~Effect();
    virtual void draw(int delta, Tileset &tiles);
    virtual void init();
    virtual void advance();
};

}
#endif
