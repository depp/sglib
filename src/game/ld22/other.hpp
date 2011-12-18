#ifndef GAME_LD22_OTHER_HPP
#define GAME_LD22_OTHER_HPP
#include "walker.hpp"
namespace LD22 {

class Other : public Walker {
public:
    Other(int x, int y)
    {
        m_x = x;
        m_y = y;
    }

    virtual ~Other();
    virtual void advance();
};

}
#endif
