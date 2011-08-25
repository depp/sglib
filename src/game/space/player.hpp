// Copyright 2006 Dietrich Epp <depp@zdome.net>
// $Id: player.h 51 2006-08-16 15:32:33Z depp $
#ifndef PLAYER_H
#define PLAYER_H
#include "thinker.hpp"
namespace sparks {
class ship;

enum {
	key_left,
	key_right,
	key_thrust,
	key_brake,
	key_fire
};

class player : public thinker {
	public:
		player();
		virtual ~player();
		virtual void think(game& g, double delta);
		virtual void enter_game(game& g);
		virtual void leave_game(game& g);
		void set_key(int key, bool flag);
		ship* f_ship;
	private:
		bool key_pressed(int key) const { return f_keys & (1 << key); }
		unsigned int f_keys;
		double f_next_shot_time;
};

} // namespace sparks
#endif
