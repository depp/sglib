#ifndef GAME_SPACE_GAMESCREEN_HPP
#define GAME_SPACE_GAMESCREEN_HPP
#include "ui/screen.hpp"
class RasterText;
namespace UI {
struct KeyEvent;
}
namespace Space {

class GameScreen : public UI::Screen {
public:
    virtual void handleEvent(UI::Event const &evt);
    virtual void update(unsigned int ticks);
    virtual void draw();

private:
    void handleKey(UI::KeyEvent const &evt);
};

}
#endif
