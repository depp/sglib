#ifndef GAME_TANK_GAMESCREEN_HPP
#define GAME_TANK_GAMESCREEN_HPP
#include "client/ui/screen.hpp"
#include "client/ui/keymanager.hpp"
class RasterText;
namespace Tank {
class World;

class GameScreen : public UI::Screen {
public:
    GameScreen();
    virtual ~GameScreen();

    virtual void handleEvent(UI::Event const &evt);
    virtual void update(unsigned int ticks);
    virtual void draw();

private:
    UI::KeyManager key_;
    World *world_;
};

}
#endif
