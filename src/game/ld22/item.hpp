#ifndef GAME_LD22_ITEM_HPP
#define GAME_LD22_ITEM_HPP
#include "mover.hpp"
namespace LD22 {
class Walker;

class Item : public Mover {
public:
    typedef enum {
        SFree,
        SGrabbing,
        SGrabbed
    } State;

    static const int IWIDTH = 48, IHEIGHT = 48;

    Item(int x, int y)
        : Mover(AItem), m_owner(0), m_state(SFree), m_frame(0),
          m_locked(false)
    {
        m_x = x;
        m_y = y;
        m_w = IWIDTH;
        m_h = IHEIGHT;
    }

    virtual ~Item();

    virtual void draw(int delta, Tileset &tiles);

    virtual void advance();

    // These are also manipulated by Walker
    Walker *m_owner;
    State m_state;
    int m_frame;

    // Locked items can't be grabbed
    bool m_locked;
};

}
#endif
