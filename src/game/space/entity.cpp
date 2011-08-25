// Copyright 2006 Dietrich Epp <depp@zdome.net>
// $Id: entity.cpp 51 2006-08-16 15:32:33Z depp $
#include "entity.h"
namespace sparks {

entity::entity() : radius(0.0f), layer(0) { }

entity::~entity() { }

void entity::move(game& g, double delta) { }

} // namespace synth
