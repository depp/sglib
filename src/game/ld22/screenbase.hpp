#ifndef GAME_LD22_SCREENBASE_HPP
#define GAME_LD22_SCREENBASE_HPP
#include "client/ui/screen.hpp"
#include "client/letterbox.hpp"
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
    Letterbox m_letterbox;
    int m_width, m_height;

protected:
    void setSize(int w, int h)
    {
        m_letterbox.setISize(w, h);
        m_width = w;
        m_height = h;
    }

    // Override, subclass can draw on top of level
    // orthographic projection 0..w, 0..h from setSize
    // will be set up
    virtual void drawExtra() = 0;

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
