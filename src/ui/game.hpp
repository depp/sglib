#ifndef UI_GAME_HPP
#define UI_GAME_HPP
#include "screen.hpp"
#include "game/tank/player.hpp"

namespace Tank {
class World;
}

class RasterText;
namespace UI {
struct KeyEvent;

class Game : public Screen {
public:
    Game()
        : world_(0)
    { }
    virtual ~Game();

    virtual void handleEvent(Event const &evt);
    virtual void update(unsigned int ticks);
    virtual void draw();

private:
    void handleKey(KeyEvent const &evt);

    Tank::World *world_;
    Tank::Player::Input input_;
};

}
#endif
