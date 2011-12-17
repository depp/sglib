#ifndef GAME_LD22_WALKER_HPP
#define GAME_LD22_WALKER_HPP
#include "actor.hpp"
#include "client/texture.hpp"
namespace LD22 {

/* An actor that walks around, is affected by gravity.  */
class Walker : public Actor {
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

    static const int WWIDTH = 28, WHEIGHT = 48;
    static const int SPEED_SCALE = 256;
    static const int GRAVITY = 128;
    // Current velocity, updated by this class
    int m_xspeed, m_yspeed;
    // Updated by Walker
    WState m_wstate;
    Texture::Ref m_tex;

    WAnim m_anim;
    int m_animtime;
    bool m_animlock;

    int m_sprite;

protected:
    // "Push": walking and jumping
    // subclass is responsible for updating
    int m_xpush, m_ypush;

public:
    Walker(int x, int y)
        : Actor(x, y, WWIDTH, WHEIGHT)
    { }

    virtual ~Walker();
    virtual void draw(int delta);
    virtual void init();
    virtual void advance();

private:
    // Check whether falling or standing
    void checkState();
};

}
#endif
