#ifndef GAME_SPACE_PLAYER_HPP
#define GAME_SPACE_PLAYER_HPP
#include "thinker.hpp"
#include "ship.hpp"
namespace UI {
class KeyManager;
}
namespace Space {
class Ship;

enum {
    KeyLeft,
    KeyRight,
    KeyThrust,
    KeyBrake,
    KeyFire
};

class Player : public Thinker {
    Ship *ship_;
    const UI::KeyManager &keys_;
    double nextShotTime_;

public:
    Player(const UI::KeyManager &mgr);
    virtual ~Player();
    virtual void think(World &w, double delta);

    virtual void enterGame(World &w);
    virtual void leaveGame(World &w);

    vector location()
    {
        return (this && ship_) ? ship_->location : vector();
    }

private:
    bool keyPressed(int key) const;
};

} // namespace sparks
#endif
