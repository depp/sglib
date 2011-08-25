#ifndef PLAYER_H
#define PLAYER_H
#include "thinker.hpp"
#include "ship.hpp"
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
    unsigned int keys_;
    double nextShotTime_;

public:
    Player();
    virtual ~Player();
    virtual void think(World &w, double delta);

    virtual void enterGame(World &w);
    virtual void leaveGame(World &w);
    void setKey(int key, bool flag);

    vector location()
    {
        return (this && ship_) ? ship_->location : vector();
    }

private:
    bool keyPressed(int key) const
    {
        return keys_ & (1U << key);
    }
};

} // namespace sparks
#endif
