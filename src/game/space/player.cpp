// Copyright 2006 Dietrich Epp <depp@zdome.net>
// $Id: player.cpp 51 2006-08-16 15:32:33Z depp $
#include "player.h"
#include "ship.h"
#include "game.h"
#include "shot.h"
#include <math.h>
namespace sparks {

const float k_rotation_speed = 2.0 * M_PI * 2.0;
const float k_max_speed = 30.0 * 8.0;
const float k_acceleration = 180.0 * 8.0;
const float k_brake = 36.0 * 8.0;
const float k_shot_velocity = 45.0 * 8.0;
const float k_shot_time = 25.0 / 60.0;
const float k_shot_delay = 1.0 / 60.0;

player::player() : f_ship(NULL), f_keys(0), f_next_shot_time(-1.0) { }

player::~player() { }
	
void player::think(game& g, double delta) {
	int rotation = 0;
	if (key_pressed(key_left))
		rotation += 1;
	if (key_pressed(key_right))
		rotation -= 1;
	f_ship->angle += ((float)rotation) * k_rotation_speed * delta;
	if (key_pressed(key_thrust)) {
		if (!key_pressed(key_brake)) {
			f_ship->velocity += f_ship->direction * k_acceleration * delta;
			if (f_ship->velocity.squared() > k_max_speed * k_max_speed)
				f_ship->velocity.normalize() *= k_max_speed;
		}
	} else if (key_pressed(key_brake)) {
		float delta_v = k_brake * delta;
		float vel_squared = f_ship->velocity.squared();
		if (vel_squared < delta_v * delta_v)
			f_ship->velocity = vector(0.0f, 0.0f);
		else
			f_ship->velocity -= f_ship->velocity * (delta_v / sqrtf(vel_squared));
	}
	if (key_pressed(key_fire) & (g.time >= f_next_shot_time)) {
		vector location = f_ship->location + f_ship->direction * 16.0f;
		vector velocity = f_ship->velocity + f_ship->direction * k_shot_velocity;
		g.add_thinker(new shot(location, velocity, k_shot_time));
		f_next_shot_time = g.time + k_shot_delay;
	}
}

void player::enter_game(game& g) {
	f_ship = new ship;
	f_ship->angle = M_PI_2;
	g.add_entity(f_ship);
}

void player::leave_game(game& g) {
	g.remove_entity(f_ship);
}

void player::set_key(int key, bool flag) {
	int mask = 1 << key;
	if (flag)
		f_keys |= mask;
	else
		f_keys &= ~mask;
}

} // namespace sparks
