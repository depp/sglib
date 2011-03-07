#ifndef UI_GAME_HPP
#define UI_GAME_HPP
#include "screen.hpp"
#include "game/player.hpp"
class World;
class RasterText;
namespace UI {
struct KeyEvent;

class Game : public Screen {
public:
    Game()
        : tickref_(0), world_(0),
          havefps_(false), framecount_(0), framerate_(0)
    { }
    virtual ~Game();

    virtual void handleEvent(Event const &evt);
    virtual void update(unsigned int ticks);
    virtual void draw();

private:
    void handleKey(KeyEvent const &evt);
    void initWorld();

    unsigned int tickref_;
    World *world_;
    Player::Input input_;
    bool havefps_;
    unsigned int frametick_[64];
    unsigned int framecount_;
    unsigned int framecur_;
    RasterText *framerate_;
};

}
#endif
