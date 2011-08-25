// Copyright 2006 Dietrich Epp <depp@zdome.net>
// $Id: game.cpp 51 2006-08-16 15:32:33Z depp $
#include "game.h"
#include "entity.h"
#include "thinker.h"
namespace sparks {

struct game::event {
	enum type {
		type_add_entity,
		type_remove_entity,
		type_add_thinker,
		type_remove_thinker,
	};
	type t;
	void* o;
	event(type i_t, entity* e) : t(i_t), o(reinterpret_cast<void*>(e)) { }
	event(type i_t, thinker* t) : t(i_t), o(reinterpret_cast<void*>(t)) { }
	entity* as_entity() const { return reinterpret_cast<entity*>(o); }
	thinker* as_thinker() const { return reinterpret_cast<thinker*>(o); }
};

game::game() : time(0.0) { }

game::~game() {
	for (std::set<entity*>::iterator
			i = f_entities.begin(),
			e = f_entities.end();
			i != e; ++i)
		delete *i;
	for (std::set<thinker*>::iterator
			i = f_thinkers.begin(),
			e = f_thinkers.end();
			i != e; ++i)
		delete *i;
}

void game::run_frame(double new_time) {
	double delta = new_time - time;
	time = new_time;
	for (unsigned int i = 0; i < f_events.size(); ++i) {
		event& evt = f_events[i];
		entity* e = evt.as_entity();
		thinker* t = evt.as_thinker();
		switch (evt.t) {
			case event::type_add_entity:
				f_entities.insert(e);
				break;
			case event::type_remove_entity:
				if (f_entities.erase(e))
					delete e;
				break;
			case event::type_add_thinker:
				f_thinkers.insert(t);
				t->enter_game(*this);
				break;
			case event::type_remove_thinker:
				t->leave_game(*this);
				if (f_thinkers.erase(t))
					delete t;
				break;
		}
	}
	f_events.clear();
	for (std::set<thinker*>::iterator
			i = f_thinkers.begin(),
			e = f_thinkers.end();
			i != e; ++i) {
		thinker& a = **i;
		a.think(*this, delta);
	}
	for (std::set<entity*>::iterator
			i = f_entities.begin(),
			e = f_entities.end();
			i != e; ++i) {
		entity& a = **i;
		a.move(*this, delta);
	}
}

void game::draw() {
	for (std::set<entity*>::iterator
			i = f_entities.begin(),
			e = f_entities.end();
			i != e; ++i)
		(*i)->draw();
}

void game::add_entity(entity* e) {
	f_events.push_back(event(event::type_add_entity, e));
}

void game::remove_entity(entity* e) {
	f_events.push_back(event(event::type_remove_entity, e));
}

void game::add_thinker(thinker* t) {
	f_events.push_back(event(event::type_add_thinker, t));
}

void game::remove_thinker(thinker* t) {
	f_events.push_back(event(event::type_remove_thinker, t));
}

} // namespace synth
