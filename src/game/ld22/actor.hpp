#ifndef GAME_LD22_ACTOR_HPP
#define GAME_LD22_ACTOR_HPP
#include "defs.hpp"
namespace LD22 {
class Area;
class Tileset;

class Actor {
    friend class Area;
public:
    typedef enum {
        ANone,
        AWalker,
        AItem
    } Type;

private:
    Area *m_area;
    int m_x0, m_y0;
    Type m_type;

protected:
    // These must be initialized by subclass!
    // x, y are bottom left
    int m_x, m_y, m_w, m_h;

public:

    enum {
        MoveFull,
        MovePartial,
        MoveNone
    };

    Actor(Type t)
        : m_area(0), m_type(t)
    { }

    virtual ~Actor();

    // Called with number of ms since last frame
    virtual void draw(int delta, Tileset &tiles);

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
    // Returns MoveFull, MovePartial, or MoveNone depending on how
    // far the actor moved.
    int moveTo(int x, int y);

    Area &area()
    {
        return *m_area;
    }

    Type type() const
    {
        return m_type;
    }

    int centerx() const
    {
        return m_x + m_w / 2;
    }

    int centery() const
    {
        return m_y + m_h / 2;
    }
};

}
#endif
