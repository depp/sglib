#include "entity.hpp"
namespace Space {

Entity::Entity()
  : radius(0.0f), layer(0)
{ }

Entity::~Entity()
{ }

void Entity::move(World &, double)
{ }

}
