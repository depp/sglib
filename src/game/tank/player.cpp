#include "player.hpp"
#include "world.hpp"
#include "shot.hpp"
#include "client/ui/keymanager.hpp"
#include <cmath>
namespace Tank {

const float kPlayerForwardSpeed = 10.0f;
const float kPlayerTurnSpeed = 100.0f;
const float kPlayerSize = 1.0f;
const float kShotDistance = 1.3f;

Player::Player(float x, float y, float face, const UI::KeyManager &key)
    : Object(kClassSolid, kClassSolid, x, y, face, kPlayerSize),
      key_(key), fireLatch_(false)
{ }

Player::~Player()
{ }

void Player::draw()
{ }

void Player::update()
{
    float forward = 0.0f, turn = 0.0f;
    if (key_.inputState(KeyLeft))
        turn += kPlayerTurnSpeed;
    if (key_.inputState(KeyRight))
        turn -= kPlayerTurnSpeed;
    if (key_.inputState(KeyForward))
        forward += kPlayerForwardSpeed;
    if (key_.inputState(KeyBack))
        forward -= kPlayerForwardSpeed;
    turn *= World::kFrameTime;
    setFace(getFace() + turn);
    setSpeed(forward);

    if (key_.inputState(KeyFire) && !fireLatch_) {
        float a = getFace() * (4.0f * std::atan(1.0f) / 180.0f);
        Object *obj = new Shot(getX() + std::cos(a) * kShotDistance,
                               getY() + std::sin(a) * kShotDistance,
                               getFace());
        getWorld().addObject(obj);
    }
    fireLatch_ = key_.inputState(KeyFire);
}

}
