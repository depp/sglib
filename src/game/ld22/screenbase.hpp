#ifndef GAME_LD22_SCREENBASE_HPP
#define GAME_LD22_SCREENBASE_HPP
#include "client/ui/screen.hpp"
#include <memory>
namespace LD22 {
class Area;
class Background;

class ScreenBase : public UI::Screen {
    std::auto_ptr<Background> m_background;
    std::auto_ptr<Area> m_area;
    unsigned m_tickref;
    int m_delta;
    bool m_init;

public:
    ScreenBase();
    virtual ~ScreenBase();

    virtual void init();
    virtual void update(unsigned int ticks);
    virtual void draw();

    Area &area()
    {
        return *m_area;
    }

private:
    void advance();
};

}
#endif
