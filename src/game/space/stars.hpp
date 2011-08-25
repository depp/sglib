// Copyright 2006 Dietrich Epp <depp@zdome.net>
// $Id: stars.h 51 2006-08-16 15:32:33Z depp $
#ifndef STARS_H
#define STARS_H
namespace sparks {

class starfield {
	public:
		starfield();
		void draw(float xmin, float ymin, float xsize, float ysize);
		unsigned int seed;
		float parallax;
		float tile_size;
};

} // namespace sparks
#endif
