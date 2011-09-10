#ifndef UI_WINDOW_HPP
#define UI_WINDOW_HPP
/* A Window object is the interface between platform code and common code.  */
#include "screen.hpp"
namespace UI {

class Window {
    Screen *screen_;
    unsigned width_;
    unsigned height_;

public:
    Window()
        : screen_(0), width_(0), height_(0)
    { }

    ~Window();

    void setScreen(Screen *s);
    void setSize(unsigned w, unsigned h);

    void handleEvent(Event const &evt)
    {
        screen_->handleEvent(evt);
    }

    void draw();

    unsigned width()
    {
        return width_;
    }

    unsigned height()
    {
        return height_;
    }
};

}
#endif
