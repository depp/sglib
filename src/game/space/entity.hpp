// Copyright 2006 Dietrich Epp <depp@zdome.net>
// $Id: entity.h 51 2006-08-16 15:32:33Z depp $
#ifndef ENTITY_H
#define ENTITY_H
#include "vector.hpp"
namespace sparks {
class game;

class entity {
	public:
		entity();
		virtual ~entity();
		vector location;
		float radius;
		int layer;
		virtual void move(game& g, double delta);
		virtual void draw() = 0;
};

} // namespace synth
#endif
