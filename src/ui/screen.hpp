#ifndef UI_SCREEN_HPP
#define UI_SCREEN_HPP
namespace UI {
struct Event;

class Screen {
private:
    static Screen *active;

public:
    static void setActive(Screen *screen);
    static Screen *getActive() { return active; }

    virtual ~Screen();
    virtual void handleEvent(Event const &evt) = 0;
    virtual void update(unsigned int ticks) = 0;
    virtual void draw() = 0;
};

}
#endif
