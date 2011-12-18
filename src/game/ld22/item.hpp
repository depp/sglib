#ifndef GAME_LD22_ITEM_HPP
#define GAME_LD22_ITEM_HPP
#include "mover.hpp"
namespace LD22 {
class Walker;

class Item : public Mover {
    // No longer usable to player, but still on screen (maybe)
    bool m_gone;

public:
    typedef enum {
        SFree,
        SGrabbing,
        SGrabbed,
        SEndStar
    } State;

    typedef enum {
        Star,
        Bomb,
        EndStar
    } Type;

    static const int IWIDTH = 48, IHEIGHT = 48;

    Item(int x, int y, Type t)
        : Mover(AItem), m_gone(false), m_itype(t),
          m_owner(0), m_state(SFree), m_frame(0), m_locked(false)
    {
        m_x = x;
        m_y = y;
        m_w = IWIDTH;
        m_h = IHEIGHT;
    }

    virtual ~Item();
    virtual void wasDestroyed();
    virtual void init();
    virtual void draw(int delta, Tileset &tiles);

    virtual void advance();

    // Release a grab, and make sure it's not stuck in a wall.
    void setFree();

    Type m_itype;

    // These are also manipulated by Walker
    Walker *m_owner;
    State m_state;
    int m_frame;

    // Locked items can't be grabbed
    bool m_locked;

    void markAsLost();
};

}
#endif
