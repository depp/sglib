#ifndef UI_GAME_HPP
#define UI_GAME_HPP
#include "screen.hpp"
#include "game/player.hpp"
class World;
namespace UI {
struct KeyEvent;

class Game : public Screen {
public:
    Game()
        : tickref_(0), world_(0)
    { }
    virtual ~Game();

    virtual void handleEvent(Event const &evt);
    virtual void draw(unsigned int ticks);

private:
    void handleKey(KeyEvent const &evt);
    void initWorld();

    unsigned int tickref_;
    World *world_;
    Player::Input input_;
};

}
#endif
