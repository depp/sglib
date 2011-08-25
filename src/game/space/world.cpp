#include "world.hpp"
#include "entity.hpp"
#include "thinker.hpp"
#include "stars.hpp"
#include "opengl.hpp"
#include "player.hpp"
#include "scenery.hpp"
namespace Space {

struct World::Event {
	enum Type {
		EntityAdd,
		EntityRemove,
		ThinkerAdd,
		ThinkerRemove
	};

	Type t;
	void *o;

	Event(Type tt, Entity *oo)
        : t(tt), o(reinterpret_cast<void*>(oo))
    { }

	Event(Type tt, Thinker *oo)
        : t(tt), o(reinterpret_cast<void*>(oo))
    { }

	Entity *asEntity() const
    {
        return reinterpret_cast<Entity*>(o);
    }

	Thinker *asThinker() const
    {
        return reinterpret_cast<Thinker *>(o);
    }
};

World::World()
  : time_(0.0)
{
    starfields_.resize(9);
    for (int i = 0; i < 9; ++i) {
        starfields_[i].parallax = 0.1 * (i + 1);
        starfields_[i].tileSize = 64.0;
    }

    player_ = new Player();
    addThinker(player_);

    Scenery *s = new Scenery;
    s->radius = 32.0f;
    addEntity(s);
}

World::~World()
{
	for (std::set<Entity*>::iterator
             i = entities_.begin(), e = entities_.end();
         i != e; ++i)
		delete *i;
	for (std::set<Thinker*>::iterator
             i = thinkers_.begin(), e = thinkers_.end();
         i != e; ++i)
		delete *i;
}

void World::update(unsigned ticks)
{
    double new_time = 0.001 * ticks;
	double delta = new_time - time_;
	time_ = new_time;
	for (unsigned int i = 0; i < events_.size(); ++i) {
		Event &evt = events_[i];
		Entity *e = evt.asEntity();
		Thinker *t = evt.asThinker();
		switch (evt.t) {
        case Event::EntityAdd:
            entities_.insert(e);
            break;
        case Event::EntityRemove:
            if (entities_.erase(e))
                delete e;
            break;
        case Event::ThinkerAdd:
            thinkers_.insert(t);
            t->enterGame(*this);
            break;
        case Event::ThinkerRemove:
            t->leaveGame(*this);
            if (thinkers_.erase(t))
                delete t;
            break;
		}
	}
	events_.clear();
	for (std::set<Thinker *>::iterator
             i = thinkers_.begin(),
             e = thinkers_.end();
         i != e; ++i) {
		Thinker &a = **i;
		a.think(*this, delta);
	}
	for (std::set<Entity*>::iterator
             i = entities_.begin(),
             e = entities_.end();
         i != e; ++i) {
		Entity &a = **i;
		a.move(*this, delta);
	}
}

void World::draw()
{
    glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-320.0, 320.0, -240.0, 240.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);

    vector loc = player_->location();
    glLoadIdentity();
    glTranslatef(-loc.v[0], -loc.v[1], 0.0f);

    for (std::vector<Starfield>::iterator
             i = starfields_.begin(), e = starfields_.end();
         i != e; ++i)
        i->draw(loc.v[0] - 320.0, loc.v[1] - 240.0, 640.0, 480.0);

	for (std::set<Entity*>::iterator
             i = entities_.begin(),
             e = entities_.end();
         i != e; ++i)
		(*i)->draw();
}

void World::addEntity(Entity *e)
{
	events_.push_back(Event(Event::EntityAdd, e));
}

void World::removeEntity(Entity *e)
{
	events_.push_back(Event(Event::EntityRemove, e));
}

void World::addThinker(Thinker* t)
{
	events_.push_back(Event(Event::ThinkerRemove, t));
}

void World::removeThinker(Thinker* t)
{
	events_.push_back(Event(Event::ThinkerRemove, t));
}

}
