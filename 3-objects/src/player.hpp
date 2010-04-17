#ifndef PLAYER_HPP
#define PLAYER_HPP
#include "object.hpp"

class Player : public Object {
public:
    struct Input {
        bool left;
        bool right;
        bool up;
        bool down;
    };

    Player(float x, float y, float face, Input &input);
    virtual ~Player();
    virtual void draw();
    virtual void update();

private:
    Input &input_;
};

#endif
