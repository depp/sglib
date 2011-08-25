// Copyright 2006 Dietrich Epp <depp@zdome.net>
// $Id: main.cpp 51 2006-08-16 15:32:33Z depp $
#include <SDL/SDL.h>
#include <GL/gl.h>
#include <stdlib.h>
#include <time.h>
#include "player.hpp"
#include "game.hpp"
#include "scenery.hpp"
#include "stars.hpp"
#include "ship.hpp"
using namespace sparks;

static void init() {
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		fprintf(stderr, "could not initialize SDL: %s\n", SDL_GetError());
		exit(1);
	}
	int value = 1;
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, value);
	unsigned int flags = SDL_OPENGL;// | SDL_FULLSCREEN;
	SDL_Surface* screen = SDL_SetVideoMode(640, 480, 0, flags);
	if (!screen) {
		fprintf(stderr, "could not set screen size: %s\n", SDL_GetError());
		SDL_Quit();
		exit(1);
	}
	glViewport(0, 0, 640, 480);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glMatrixMode(GL_PROJECTION);
	glOrtho(-320.0, 320.0, -240.0, 240.0, -1.0, 1.0);
	srand(time(NULL));
}

static void term() {
	SDL_Quit();
	exit(0);
}

class main_looper {
	public:
		game f_game;
		player* f_player;
		bool f_done;
		int f_start_tick;
		std::vector<starfield> f_stars;
		main_looper() : f_game(), f_player(NULL), f_done(false) {
			f_start_tick = SDL_GetTicks();
			f_player = new player;
			f_game.add_thinker(f_player);
			scenery* s = new scenery;
			s->radius = 32.0f;
			f_game.add_entity(s);
			for (int i = 0; i < 9; ++i) {
				f_stars.push_back(starfield());
				f_stars[i].parallax = 0.1 * (float)(i + 1);
				f_stars[i].tile_size = 64.0;
			}
		}
		void key(int key, bool flag) {
			int player_key = -1;
			switch (key) {
				case SDLK_LEFT:
					player_key = key_left;
					break;
				case SDLK_RIGHT:
					player_key = key_right;
					break;
				case SDLK_UP:
					player_key = key_thrust;
					break;
				case SDLK_DOWN:
					player_key = key_brake;
					break;
				case SDLK_SPACE:
					player_key = key_fire;
					break;
				case SDLK_q:
				case SDLK_ESCAPE:
					f_done = true;
					break;
			}
			if (player_key < 0)
				return;
			f_player->set_key(player_key, flag);
		}
		void run() {
			unsigned int last_tick = f_start_tick;
			SDL_Event event;
			for (;;) {
				if (f_done)
					return;
				while (SDL_PollEvent(&event)) {
					switch (event.type) {
						case SDL_MOUSEMOTION:
							break;
						case SDL_MOUSEBUTTONDOWN:
							break;
						case SDL_KEYDOWN:
							key(event.key.keysym.sym, true);
							break;
						case SDL_KEYUP:
							key(event.key.keysym.sym, false);
							break;
						case SDL_QUIT:
							break;
						default:
							break;
					}
				}
				if (f_done)
					return;
				unsigned int current_tick = SDL_GetTicks();
				if (current_tick <= last_tick)
					continue;
				last_tick = current_tick;
				f_game.run_frame(0.001*static_cast<double>(current_tick - f_start_tick));
				glClear(GL_COLOR_BUFFER_BIT);
				glMatrixMode(GL_PROJECTION);
				glPushMatrix();
				vector& loc = f_player->f_ship->location;
				glTranslatef(-loc.v[0], -loc.v[1], 0.0f);
				for (std::vector<starfield>::iterator i = f_stars.begin(), e = f_stars.end();
						i != e; ++i)
					i->draw(loc.v[0] - 320.0, loc.v[1] - 240.0, 640.0, 480.0);
				f_game.draw();
				glMatrixMode(GL_PROJECTION);
				glPopMatrix();
				SDL_GL_SwapBuffers();
			}
		}
};

int main(int argc, char** argv) {
	init();
	main_looper().run();
	term();
	return 0;
}
