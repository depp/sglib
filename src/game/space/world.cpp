#include "world.hpp"
#include "entity.hpp"
#include "thinker.hpp"
#include "stars.hpp"
#include "opengl.hpp"
#include "player.hpp"
#include "graphics/video.hpp"
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
    starfields_.reserve(9);
    for (int i = 0; i < 9; ++i) {
        starfields_.push_back(Starfield());
        starfields_[i].parallax = 0.1 * (i + 1);
        starfields_[i].tileSize = 64.0;
    }
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
        Event evt = events_[i];
        Entity *e;
        Thinker *t;

        switch (evt.t) {
        case Event::EntityAdd:
            e = evt.asEntity();
            entities_.insert(e);
            break;

        case Event::EntityRemove:
            e = evt.asEntity();
            if (entities_.erase(e))
                delete e;
            break;

        case Event::ThinkerAdd:
            t = evt.asThinker();
            thinkers_.insert(t);
            t->enterGame(*this);
            break;

        case Event::ThinkerRemove:
            t = evt.asThinker();
            if (thinkers_.erase(t)) {
                t->leaveGame(*this);
                delete t;
            }
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
    int w = Video::width, h = Video::height;
    glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-w/2, w-w/2, -h/2, h-h/2, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);

    vector loc = player_->location();
    glLoadIdentity();
    glTranslatef(-loc.v[0], -loc.v[1], 0.0f);

    for (std::vector<Starfield>::iterator
             i = starfields_.begin(), e = starfields_.end();
         i != e; ++i)
        i->draw(loc.v[0] - w/2, loc.v[1] - h/2, w, h);

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
    events_.push_back(Event(Event::ThinkerAdd, t));
}

void World::removeThinker(Thinker* t)
{
    events_.push_back(Event(Event::ThinkerRemove, t));
}

}
