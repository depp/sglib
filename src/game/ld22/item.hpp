#ifndef GAME_LD22_ITEM_HPP
#define GAME_LD22_ITEM_HPP
#include "mover.hpp"
namespace LD22 {

class Item : public Mover {
public:
    static const int IWIDTH = 32, IHEIGHT = 32;

    Item(int x, int y)
    {
        m_x = x;
        m_y = y;
        m_w = IWIDTH;
        m_h = IHEIGHT;
    }

    virtual ~Item();

    virtual void draw(int delta, Tileset &tiles);

    virtual void advance();
};

}
#endif
