// Copyright 2006 Dietrich Epp <depp@zdome.net>
// $Id: game.h 51 2006-08-16 15:32:33Z depp $
#ifndef GAME_H
#define GAME_H
#include <vector>
#include <set>
namespace sparks {
class entity;
class thinker;

class game {
	public:
		game();
		~game();
		void run_frame(double new_time);
		void draw();
		void add_entity(entity* e);
		void remove_entity(entity* e);
		void add_thinker(thinker* t);
		void remove_thinker(thinker* t);
		double time;
	private:
		struct event;
		std::set<entity*> f_entities;
		std::set<thinker*> f_thinkers;
		std::vector<event> f_events;
};

} // namespace sparks
#endif
