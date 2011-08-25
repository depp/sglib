// Copyright 2006 Dietrich Epp <depp@zdome.net>
// $Id: stars.cpp 51 2006-08-16 15:32:33Z depp $
#include <GL/gl.h>
#include "stars.hpp"
#include "random.hpp"
#include <stdlib.h>
#include <math.h>
namespace sparks {

static const unsigned char k_star_colors[6][3] = {
	{ 255, 219, 219 }, { 255, 255, 219 }, { 219, 255, 219 },
	{ 219, 255, 255 }, { 219, 219, 255 }, { 255, 219, 255 },
};

struct tile {
	unsigned short v[2];
	unsigned char color;
	inline void randomize(unsigned int seed, short x_n, short y_n) {
		unsigned int data = ((x_n & 0xffff) << 16) | (y_n & 0xffff);
		data = stateless_random(seed, data);
		v[0] = (data << 4) & 0xffff;
		v[1] = (data >> 8) & 0xffff;
		color = (data >> 24) & 0xff;
	}
};

starfield::starfield() : seed(rand()) { }

void starfield::draw(float xmin, float ymin, float xsize, float ysize) {
	float scale = tile_size / 65536.0f,
		pxmin = xmin * parallax,
		pymin = ymin * parallax,
		x_minf = floorf(pxmin / tile_size),
		y_minf = floorf(pymin / tile_size);
	int s = seed,
		x_min = (int)x_minf,
		x_max = (int)ceilf((pxmin + xsize) / tile_size),
		y_min = (int)y_minf,
		y_max = (int)ceilf((pymin + ysize) / tile_size);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glTranslatef(
		x_minf * tile_size + xmin * (1.0f - parallax),
		y_minf * tile_size + ymin * (1.0f - parallax),
		0.0f);
	glScalef(scale, scale, 1.0f);
	glPointSize(1.0f);
	glBegin(GL_POINTS);
	for (int x = x_min, xn = 0; x <= x_max; ++x, ++xn) {
		for (int y = y_min, yn = 0; y <= y_max; ++y, ++yn) {
			tile t;
			t.randomize(s, x, y);
			glColor3ubv(k_star_colors[t.color % 6]);
			//cout << "(" << xn << ", " << yn << ") -> (" << t.v[0] << ", " << t.v[1] << ")" << endl;
			int x_i = (xn << 16) + t.v[0],
				y_i = (yn << 16) + t.v[1];
			glVertex2i(x_i, y_i);
			//glColor3ub(255, 0, 0);
			//glVertex2i(xn << 16, yn << 16);
		}
	}
	glEnd();
	glPopMatrix();
}

} // namespace sparks
