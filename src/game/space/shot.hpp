// Copyright 2006 Dietrich Epp <depp@zdome.net>
// $Id: shot.h 51 2006-08-16 15:32:33Z depp $
#ifndef SHOT_H
#define SHOT_H
#include "thinker.hpp"
namespace sparks {
class vector;

class shot : public thinker {
	public:
		shot(vector const& location, vector const& velocity, double time);
		virtual ~shot();
		virtual void think(game& g, double delta);
		virtual void enter_game(game& g);
		virtual void leave_game(game& g);
	private:
		class shot_entity;
		shot_entity* f_shot;
		double f_endtime;
};

} // namespace sparks
#endif
