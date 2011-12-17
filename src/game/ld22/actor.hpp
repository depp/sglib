#ifndef GAME_LD22_ACTOR_HPP
#define GAME_LD22_ACTOR_HPP
namespace LD22 {
class Area;

class Actor {
public:
    int m_x, m_y;

    Actor(int x, int y)
        : m_x(x), m_y(y)
    { }

    virtual ~Actor();

    virtual void draw();
    virtual void advance();
};

}
#endif
