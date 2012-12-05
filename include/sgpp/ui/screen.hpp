#ifndef CLIENT_UI_SCREEN_HPP
#define CLIENT_UI_SCREEN_HPP
class Viewport;
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
    virtual void draw(Viewport &v, unsigned msec) = 0;

    // Make the screen the active screen and delete the current
    // active screen.
    static void makeActive(Screen *s);

    // Quit the game.
    static void quit();
};

}

// This must be implemented by the user
UI::Screen *getMainScreen();

#endif
