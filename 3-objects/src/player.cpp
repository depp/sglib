#include "player.hpp"
#include "world.hpp"
#include <cmath>

const float kPlayerForwardSpeed = 10.0f;
const float kPlayerTurnSpeed = 100.0f;

Player::Player(float x, float y, float face, Input &input)
    : Object(x, y, face), input_(input)
{ }

Player::~Player()
{ }

void Player::draw()
{ }

void Player::update()
{
    float forward = 0.0f, turn = 0.0f, face;
    if (input_.left)
        turn += kPlayerTurnSpeed;
    if (input_.right)
        turn -= kPlayerTurnSpeed;
    if (input_.up)
        forward += kPlayerForwardSpeed;
    if (input_.down)
        forward -= kPlayerForwardSpeed;
    turn *= World::kFrameTime;
    setFace(getFace() + turn);
    setSpeed(forward);
}
