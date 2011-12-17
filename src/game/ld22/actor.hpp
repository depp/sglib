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

    /* Test whether actor hits a wall at the given location */
    bool wallAt(int x, int y);

    /* Move actor to new coordinates, or partway if something is hit.
       Returns true if the actor hits something and stops partway.  */
    bool moveTo(int x, int y);
};

}
#endif
