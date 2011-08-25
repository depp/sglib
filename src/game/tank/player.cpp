#include "player.hpp"
#include "world.hpp"
#include "shot.hpp"
#include <cmath>
namespace Tank {

const float kPlayerForwardSpeed = 10.0f;
const float kPlayerTurnSpeed = 100.0f;
const float kPlayerSize = 1.0f;
const float kShotDistance = 1.3f;

Player::Player(float x, float y, float face, Input &input)
    : Object(kClassSolid, kClassSolid, x, y, face, kPlayerSize),
      input_(input)
{ }

Player::~Player()
{ }

void Player::draw()
{ }

void Player::update()
{
    float forward = 0.0f, turn = 0.0f;
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

    if (input_.fire) {
        input_.fire = false;
        float a = getFace() * (4.0 * std::atan(1.0) / 180.0f);
        Object *obj = new Shot(getX() + std::cos(a) * kShotDistance,
                               getY() + std::sin(a) * kShotDistance,
                               getFace());
        getWorld().addObject(obj);
    }
}

}
