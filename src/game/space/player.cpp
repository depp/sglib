#include "player.hpp"
#include "ship.hpp"
#include "world.hpp"
#include "shot.hpp"
#include "client/ui/keymanager.hpp"
#include <math.h>
namespace Space {

const float RotationSpeed = 2.0f * (4.0f*atan(1.0f)) * 2.0f;
const float MaxSpeed = 30.0f * 8.0f;
const float Acceleration = 180.0f * 8.0f;
const float Brake = 36.0f * 8.0f;
const float ShotVelocity = 45.0f * 8.0f;
const float ShotTime = 25.0f / 60.0f;
const float ShotDelay = 1.0f / 60.0f;

Player::Player(const UI::KeyManager &mgr)
  : ship_(NULL), keys_(mgr), nextShotTime_(-1.0)
{ }

Player::~Player()
{ }

void Player::think(World &w, double delta)
{
    int rotation = 0;
    if (keyPressed(KeyLeft))
        rotation += 1;
    if (keyPressed(KeyRight))
        rotation -= 1;
    ship_->angle += ((float)rotation) * RotationSpeed * (float) delta;
    if (keyPressed(KeyThrust)) {
        if (!keyPressed(KeyBrake)) {
            ship_->velocity += ship_->direction * Acceleration * (float) delta;
            if (ship_->velocity.squared() > MaxSpeed * MaxSpeed)
                ship_->velocity.normalize() *= MaxSpeed;
        }
    } else if (keyPressed(KeyBrake)) {
        float delta_v = Brake * (float) delta;
        float vel_squared = ship_->velocity.squared();
        if (vel_squared < delta_v * delta_v)
            ship_->velocity = vector(0.0f, 0.0f);
        else
            ship_->velocity -= ship_->velocity *
                (delta_v / sqrtf(vel_squared));
    }
    if (keyPressed(KeyFire) & (w.time() >= nextShotTime_)) {
        vector location = ship_->location + ship_->direction * 16.0f;
        vector velocity = ship_->velocity + ship_->direction * ShotVelocity;
        w.addThinker(new Shot(location, velocity, ShotTime));
        nextShotTime_ = w.time() + ShotDelay;
    }
}

void Player::enterGame(World &w)
{
    ship_ = new Ship;
    ship_->angle = 2.0f * atanf(1.0f);
    w.addEntity(ship_);
}

void Player::leaveGame(World &w)
{
    w.removeEntity(ship_);
}

bool Player::keyPressed(int key) const
{
    return keys_.inputState(key);
}

}
