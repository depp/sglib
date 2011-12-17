#ifndef GAME_LD22_ACTOR_HPP
#define GAME_LD22_ACTOR_HPP
namespace LD22 {
class Area;

class Actor {
public:
    // x, y are bottom left
    int m_x, m_y, m_w, m_h;
    Area *m_area;

    Actor(int x, int y, int w, int h)
        : m_x(x), m_y(y), m_w(w), m_h(h), m_area(0)
    { }

    virtual ~Actor();

    virtual void draw();
    virtual void advance();

    /* Can this actor move to the given location? */
    bool canMove(int x, int y);
};

}
#endif
