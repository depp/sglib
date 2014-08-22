/* Copyright 2012-2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "sg/clock.h"
#include "sg/cvar.h"
#include "sg/entry.h"
#include "sg/error.h"
#include "sg/event.h"
#include "sg/opengl.h"
#include "sg/version.h"
#include "../private.h"
#include "SDL.h"
#include <getopt.h>
#include <stdio.h>

static int sg_window_width, sg_window_height;

__attribute__((noreturn))
static void
sdl_quit(int status)
{
    SDL_Quit();
    exit(status);
}

void
sg_sys_quit(void)
{
    sdl_quit(0);
}

void
sg_sys_abort(const char *msg)
{
    fprintf(stderr, "error: %s\n", msg);
    sdl_quit(1);
}

void
sg_version_platform(struct sg_logger *lp)
{
    char v1[16], v2[16];
    const SDL_version *v = SDL_Linked_Version();
    snprintf(v1, sizeof(v1), "%d.%d.%d",
             SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL);
    snprintf(v2, sizeof(v2), "%d.%d.%d",
             v->major, v->minor, v->patch);
    sg_version_lib(lp, "LibSDL", v1, v2);
}

__attribute__((noreturn))
static void
sdl_error(const char *what)
{
    fprintf(stderr, "error: %s: %s\n", what, SDL_GetError());
    sdl_quit(1);
}

static SDL_Surface *sdl_surface;

static void
sdl_init(int argc, char *argv[])
{
    GLenum err;
    union sg_event evt;
    struct sg_game_info gameinfo;
    int opt;

    while ((opt = getopt(argc, argv, "d:")) != -1) {
        switch (opt) {
        case 'd':
            sg_cvar_addarg(NULL, NULL, optarg);
            break;

        default:
            fputs("Invalid usage\n", stderr);
            exit(1);
        }
    }

    if (SDL_Init(SDL_INIT_VIDEO))
        sdl_error("could not initialize LibSDL");

    sg_sys_init();
    sg_sys_getinfo(&gameinfo);
    sdl_surface = SDL_SetVideoMode(
        gameinfo.default_width, gameinfo.default_height,
        32, SDL_OPENGL);
    if (!sdl_surface)
        sdl_error("could not set video mode");
    err = glewInit();
    if (err)
        sg_sys_abort("could not initialize GLEW");
    sg_window_width = sdl_surface->w;
    sg_window_height = sdl_surface->h;
    evt.type = SG_EVENT_VIDEO_INIT;
    sg_game_event(&evt);
}

static void
sdl_event_mousemove(SDL_MouseMotionEvent *e)
{
    struct sg_event_mouse ee;
    ee.type = SG_EVENT_MMOVE;
    ee.button = -1;
    ee.x = e->x;
    ee.y = sg_window_height - 1 - e->y;
    sg_game_event((union sg_event *) &ee);
}

static void
sdl_event_mousebutton(SDL_MouseButtonEvent *e)
{
    struct sg_event_mouse ee;
    ee.type = e->type == SDL_MOUSEBUTTONDOWN ?
        SG_EVENT_MDOWN : SG_EVENT_MUP;
    switch (e->button) {
    case SDL_BUTTON_LEFT:   ee.button = SG_BUTTON_LEFT;   break;
    case SDL_BUTTON_MIDDLE: ee.button = SG_BUTTON_MIDDLE; break;
    case SDL_BUTTON_RIGHT:  ee.button = SG_BUTTON_RIGHT;  break;
    default:
        ee.button = SG_BUTTON_OTHER + e->button - 4;
        break;
    }
    ee.x = e->x;
    ee.y = sg_window_height - 1 - e->y;
    sg_game_event((union sg_event *) &ee);
}

static void
sdl_event_key(SDL_KeyboardEvent *e)
{
    struct sg_event_key ee;
    ee.type = e->type == SDL_KEYDOWN ?
        SG_EVENT_KDOWN : SG_EVENT_KUP;
    ee.key = -1;
    sg_game_event((union sg_event *) &ee);
}

static void
sdl_event_resize(SDL_ResizeEvent *e)
{
    sg_window_width = e->w;
    sg_window_height = e->h;
}

static void
sdl_main(void)
{
    SDL_Event e;
    while (1) {
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
            case SDL_QUIT:
                return;

            case SDL_MOUSEMOTION:
                sdl_event_mousemove(&e.motion);
                break;

            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
                sdl_event_mousebutton(&e.button);
                break;

            case SDL_KEYDOWN:
            case SDL_KEYUP:
                sdl_event_key(&e.key);
                break;

            case SDL_VIDEORESIZE:
                sdl_event_resize(&e.resize);
                break;

            default:
                break;
            }
        }

        sg_game_draw(sg_window_width, sg_window_height, sg_clock_get());
        SDL_GL_SwapBuffers();
    }
}

int
main(int argc, char *argv[])
{
    sdl_init(argc, argv);
    sdl_main();
    return 0;
}
