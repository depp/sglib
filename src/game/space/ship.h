// Copyright 2006 Dietrich Epp <depp@zdome.net>
// $Id: ship.h 51 2006-08-16 15:32:33Z depp $
#ifndef SHIP_H
#define SHIP_H
#include "entity.h"
namespace sparks {

class ship : public entity {
	public:
		ship();
		virtual ~ship();
		virtual void move(game& g, double delta);
		virtual void draw();
		float angle;
		float angular_velocity;
		vector direction;
		vector velocity;
};

} // namespace sparks
#endif
