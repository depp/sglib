// Copyright 2006 Dietrich Epp <depp@zdome.net>
// $Id: thinker.h 43 2006-08-15 22:54:22Z depp $
#ifndef THINKER_H
#define THINKER_H
namespace sparks {
class game;

class thinker {
	public:
		virtual ~thinker();
		virtual void think(game& g, double delta) = 0;
		virtual void enter_game(game& g);
		virtual void leave_game(game& g);
};

} // namespace sparks
#endif
