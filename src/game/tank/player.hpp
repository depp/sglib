#ifndef GAME_TANK_PLAYER_HPP
#define GAME_TANK_PLAYER_HPP
#include "object.hpp"
namespace UI {
class KeyManager;
}
namespace Tank {

enum {
    KeyForward,
    KeyLeft,
    KeyRight,
    KeyBack,
    KeyFire
};

class Player : public Object {
public:
    Player(float x, float y, float face, const UI::KeyManager &key);
    virtual ~Player();
    virtual void draw();
    virtual void update();

private:
    const UI::KeyManager &key_;
    bool fireLatch_;
};

}
#endif
