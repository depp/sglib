#include <GL/gl.h>
#include "scenery.hpp"
#include "shapes.hpp"
namespace Space {

Scenery::Scenery()
{ }

Scenery::~Scenery() { }

void Scenery::draw() {
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glTranslatef(location.v[0], location.v[1], 0.0f);
	glScalef(radius, radius, 1.0f);
	
	glColor3ub(0, 109, 0);
	glBegin(GL_POLYGON);
	drawCircle();
	glEnd();
	
	glLineWidth(3.0f);
	glColor3ub(0, 255, 0);
	glBegin(GL_LINE_LOOP);
	drawCircle();
	glEnd();
	
	glPopMatrix();
}

}
