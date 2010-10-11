#ifndef UI_SCREEN_HPP
#define UI_SCREEN_HPP
namespace UI {
struct Event;

class Screen {
public:
    static Screen *active;
    static void setActive(Screen *screen);

    virtual ~Screen();
    virtual void handleEvent(Event const &evt) = 0;
    virtual void draw(unsigned int ticks) = 0;
};

}
#endif
