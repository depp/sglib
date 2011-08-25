// Copyright 2006 Dietrich Epp <depp@zdome.net>
// $Id: shapes.cpp 51 2006-08-16 15:32:33Z depp $
#include <GL/gl.h>
#include "shapes.hpp"
namespace sparks {

static const int point_count = 6;
static const float points[point_count + 1] = {
	0.0, 0.258819045103, 0.5, 0.707106781187,
	0.866025403784, 0.965925826289, 1.0
};

void draw_circle() {
	for (int i = 0; i < point_count; ++i)
		glVertex2f(points[point_count-i], points[i]);
	for (int i = 0; i < point_count; ++i)
		glVertex2f(-points[i], points[point_count-i]);
	for (int i = 0; i < point_count; ++i)
		glVertex2f(-points[point_count-i], -points[i]);
	for (int i = 0; i < point_count; ++i)
		glVertex2f(points[i], -points[point_count-i]);
}

} // namespace sparks
