#ifndef CLIENT_UI_SCREEN_HPP
#define CLIENT_UI_SCREEN_HPP
namespace UI {
struct Event;
class Window;

class Screen {
    friend class Window;
    Window *window_;

public:
    Screen()
        : window_(0)
    { }

    virtual ~Screen();
    virtual void init();
    virtual void handleEvent(Event const &evt) = 0;
    virtual void update(unsigned int ticks) = 0;
    virtual void draw() = 0;

    Window &window()
    {
        return *window_;
    }
};

}
#endif
