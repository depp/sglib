#ifndef GAME_TANK_PLAYER_HPP
#define GAME_TANK_PLAYER_HPP
#include "object.hpp"
namespace Tank {

class Player : public Object {
public:
    struct Input {
        Input()
            : left(false), right(false),
              up(false), down(false), fire(false)
        { }

        bool left;
        bool right;
        bool up;
        bool down;
        bool fire;
    };

    Player(float x, float y, float face, Input &input);
    virtual ~Player();
    virtual void draw();
    virtual void update();

private:
    Input &input_;
};

}
#endif
