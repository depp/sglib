// Copyright 2006 Dietrich Epp <depp@zdome.net>
// $Id: shot.cpp 51 2006-08-16 15:32:33Z depp $
#include <GL/gl.h>
#include "shot.h"
#include "entity.h"
#include "game.h"
namespace sparks {

class shot::shot_entity : public entity {
	public:
		shot_entity();
		virtual ~shot_entity();
		virtual void move(game& g, double delta);
		virtual void draw();
		vector velocity;
};

shot::shot(vector const& location, vector const& velocity, double time) {
	f_shot = new shot_entity;
	f_shot->location = location;
	f_shot->velocity = velocity;
	f_endtime = time;
}

shot::~shot() { }

void shot::think(game& g, double delta) {
	if (g.time > f_endtime)
		g.remove_thinker(this);
}

void shot::enter_game(game& g) {
	f_endtime += g.time;
	g.add_entity(f_shot);
}

void shot::leave_game(game& g) {
	g.remove_entity(f_shot);
}

shot::shot_entity::shot_entity() { }

shot::shot_entity::~shot_entity() { }

void shot::shot_entity::move(game& g, double delta) {
	location += velocity * delta;
}

void shot::shot_entity::draw() {
	glColor3ub(0, 255, 255);
	glPointSize(2);
	glBegin(GL_POINTS);
	glVertex2f(location.v[0], location.v[1]);
	glEnd();
}

} // namespace sparks
