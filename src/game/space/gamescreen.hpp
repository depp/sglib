#ifndef GAME_SPACE_GAMESCREEN_HPP
#define GAME_SPACE_GAMESCREEN_HPP
#include "client/ui/screen.hpp"
#include "client/ui/keymanager.hpp"
class RasterText;
namespace UI {
struct KeyEvent;
}
namespace Space {
class World;
class Player;

class GameScreen : public UI::Screen {
public:
    GameScreen();

    virtual void handleEvent(UI::Event const &evt);
    virtual void update(unsigned int ticks);
    virtual void draw();

private:
    UI::KeyManager kmgr_;
    World *world_;
    Player *player_;
};

}
#endif
