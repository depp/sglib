#ifndef GAME_LD22_WALKER_HPP
#define GAME_LD22_WALKER_HPP
#include "mover.hpp"
namespace LD22 {

// An stick figure actor that walks around, is affected by gravity.
class Walker : public Mover {
public:
    // All push should be in -PUSH_SCALE .. +PUSH_SCALE
    static const int PUSH_SCALE = 256;

private:
    typedef enum {
        WFall,
        WStand
    } WState;

    typedef enum {
        AStand,
        // AWaveLeft,
        // AWaveRight,
        AFall,
        ALandLeft,
        ALandRight,
        AWalkLeft,
        AWalkRight
    } WAnim;


    WState m_wstate;
    WAnim m_anim;
    int m_animtime;
    bool m_animlock;
    int m_sprite;

protected:
    // "Push": walking and jumping
    // subclass is responsible for updating
    // These are desired speeds as a fraction of maximum
    int m_xpush, m_ypush;

public:
    Walker()
        : m_xpush(0), m_ypush(0)
    {
        m_w = STICK_WIDTH;
        m_h = STICK_HEIGHT;
    }

    virtual ~Walker();
    virtual void draw(int delta, Tileset &tiles);
    virtual void init();
    virtual void advance();

private:
    // Check whether falling or standing
    void checkState();
};

}
#endif
