#ifndef UI_GAME_HPP
#define UI_GAME_HPP
#include "ui/screen.hpp"
#include "player.hpp"
class RasterText;
namespace UI {
struct KeyEvent;
}
namespace Tank {
class World;

class GameScreen : public UI::Screen {
public:
    GameScreen()
        : world_(0)
    { }
    virtual ~GameScreen();

    virtual void handleEvent(UI::Event const &evt);
    virtual void update(unsigned int ticks);
    virtual void draw();

private:
    void handleKey(UI::KeyEvent const &evt);

    World *world_;
    Player::Input input_;
};

}
#endif
