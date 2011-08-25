// Copyright 2006 Dietrich Epp <depp@zdome.net>
// $Id: scenery.h 51 2006-08-16 15:32:33Z depp $
#ifndef SCENERY_H
#define SCENERY_H
#include "entity.hpp"
namespace sparks {

class scenery : public entity {
	public:
		scenery();
		virtual ~scenery();
		virtual void draw();
};

} // namespace sparks
#endif
