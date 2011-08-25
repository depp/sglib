// Copyright 2006 Dietrich Epp <depp@zdome.net>
// $Id: ship.cpp 51 2006-08-16 15:32:33Z depp $
#include <GL/gl.h>
#include "ship.hpp"
#include "shapes.hpp"
#include <math.h>
namespace sparks {

ship::ship() : angle(0.0f), angular_velocity(0.0f), direction(), velocity() { }

ship::~ship() { }

void ship::move(game& g, double delta) {
	location += velocity * delta;
	angle += angular_velocity * delta;
	direction.v[0] = cosf(angle);
	direction.v[1] = sinf(angle);
}

void ship::draw() {
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glTranslatef(location.v[0], location.v[1], 0.0f);
	glRotatef(angle * 180.0f * M_1_PI, 0.0f, 0.0f, 1.0f);
	
	glShadeModel(GL_SMOOTH);
	glBegin(GL_TRIANGLES);
	glColor3ub(119, 119, 119);
	glVertex2f(-16.0f, 8.0f);
	glVertex2f(-16.0f, -8.0f);
	glColor3ub(255, 0, 0);
	glVertex2f(16.0f, 0.0f);
	glEnd();
	
	glPushMatrix();
	glScalef(22.0f, 22.0f, 1.0f);
	glShadeModel(GL_FLAT);
	glColor3ub(255, 0, 0);
	glBegin(GL_LINE_LOOP);
	draw_circle();
	glEnd();
	glPopMatrix();
	
	glPopMatrix();
}

} // namespace synth
