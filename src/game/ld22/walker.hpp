#ifndef GAME_LD22_WALKER_HPP
#define GAME_LD22_WALKER_HPP
#include "actor.hpp"
#include "client/texture.hpp"
namespace LD22 {

/* An actor that walks around, is affected by gravity.  */
class Walker : public Actor {
public:
    typedef enum {
        WFall,
        WStand
    } WState;

    static const int WWIDTH = 28, WHEIGHT = 48;
    static const int SPEED_SCALE = 256;
    static const int GRAVITY = 128;
    // Current velocity, updated by this class
    int m_xspeed, m_yspeed;
    // "Push": walking and jumping, updated by subclasses
    int m_xpush, m_ypush;
    // Updated by Walker
    WState m_wstate;
    Texture::Ref m_tex;

    Walker(int x, int y)
        : Actor(x, y, WWIDTH, WHEIGHT)
    { }

    virtual ~Walker();
    virtual void draw(int delta);
    virtual void init();
    virtual void advance();

    // Check whether falling or standing
    void checkState();
};

}
#endif
