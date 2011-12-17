#ifndef GAME_LD22_ACTOR_HPP
#define GAME_LD22_ACTOR_HPP
#include "defs.hpp"
namespace LD22 {
class Area;

class Actor {
public:
    // x, y are bottom left
    int m_x, m_y, m_w, m_h;
    int m_x0, m_y0;
    Area *m_area;

    Actor(int x, int y, int w, int h)
        : m_x(x), m_y(y), m_w(w), m_h(h), m_area(0)
    { }

    virtual ~Actor();

    // Called with number of ms since last frame
    virtual void draw(int delta);

    void getDrawPos(int *x, int *y, int delta)
    {
        *x = m_x0 + (m_x - m_x0) * delta / FRAME_TIME;
        *y = m_y0 + (m_y - m_y0) * delta / FRAME_TIME;
    }

    // Draw the hit box
    void drawHitBox(int delta);

    // Called once per frame
    // Default does nothing
    virtual void advance();

    // Called when added to area
    // Default does nothing
    virtual void init();

    // Test whether actor hits a wall at the given location
    bool wallAt(int x, int y);

    // Move actor to new coordinates, or partway if something is hit.
    // Returns true if the actor hits something and stops partway.
    bool moveTo(int x, int y);
};

}
#endif
