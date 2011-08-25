// Copyright 2006 Dietrich Epp <depp@zdome.net>
// $Id: scenery.cpp 51 2006-08-16 15:32:33Z depp $
#include <GL/gl.h>
#include "scenery.hpp"
#include "shapes.hpp"
namespace sparks {

scenery::scenery() { }

scenery::~scenery() { }

void scenery::draw() {
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glTranslatef(location.v[0], location.v[1], 0.0f);
	glScalef(radius, radius, 1.0f);
	
	glColor3ub(0, 109, 0);
	glBegin(GL_POLYGON);
	draw_circle();
	glEnd();
	
	glLineWidth(3.0f);
	glColor3ub(0, 255, 0);
	glBegin(GL_LINE_LOOP);
	draw_circle();
	glEnd();
	
	glPopMatrix();
}

} // namespace sparks
