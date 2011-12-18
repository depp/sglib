#ifndef GAME_LD22_THINKER_HPP
#define GAME_LD22_THINKER_HPP
namespace LD22 {
class Area;

class Thinker {
    friend class Area;
    Area *m_area;

protected:
    // Time until timer is called
    // Ticks down every frame
    int m_time;

public:
    Thinker()
        : m_area(0)
    { }

    virtual ~Thinker();

    // called when added to area
    virtual void init();

    // Called when timer runs out
    virtual void run() = 0;

    Area &area()
    {
        return *m_area;
    }
};

}
#endif
