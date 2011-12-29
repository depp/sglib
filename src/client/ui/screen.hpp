#ifndef CLIENT_UI_SCREEN_HPP
#define CLIENT_UI_SCREEN_HPP
namespace UI {
struct Event;

class Screen {
public:
    Screen()
    { }

    virtual ~Screen();
    virtual void init();
    virtual void handleEvent(Event const &evt) = 0;
    virtual void update(unsigned int ticks) = 0;
    virtual void draw() = 0;
};

}
#endif
