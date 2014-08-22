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

#include "sg/key.h"

static SDL_Window *sg_window;
static SDL_GLContext sg_context;

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
    SDL_version v;
    SDL_GetVersion(&v);
    snprintf(v1, sizeof(v1), "%d.%d.%d",
             SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL);
    snprintf(v2, sizeof(v2), "%d.%d.%d",
             v.major, v.minor, v.patch);
    sg_version_lib(lp, "LibSDL", v1, v2);
}

__attribute__((noreturn))
static void
sdl_error(const char *what)
{
    fprintf(stderr, "error: %s: %s\n", what, SDL_GetError());
    sdl_quit(1);
}

static void
sdl_init(int argc, char *argv[])
{
    GLenum err;
    union sg_event evt;
    struct sg_game_info gameinfo;
    int opt, flags;

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

    flags = SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS;
    if (SDL_Init(flags))
        sdl_error("could not initialize LibSDL");

    sg_sys_init();
    gameinfo = sg_game_info_defaults;
    sg_sys_getinfo(&gameinfo);

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    sg_window = SDL_CreateWindow(
        gameinfo.name,
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        gameinfo.default_width,
        gameinfo.default_height,
        SDL_WINDOW_OPENGL);
    if (!sg_window)
        sdl_error("could not open window");

    sg_context = SDL_GL_CreateContext(sg_window);
    if (!sg_context)
        sdl_error("could not create OpenGL context");

    err = glewInit();
    if (err)
        sg_sys_abort("could not initialize GLEW");

    evt.type = SG_EVENT_VIDEO_INIT;
    sg_game_event(&evt);
}

static void
sdl_event_mousemove(SDL_MouseMotionEvent *e)
{
    struct sg_event_mouse ee;
    int width, height;
    SDL_GetWindowSize(sg_window, &width, &height);
    ee.type = SG_EVENT_MMOVE;
    ee.button = -1;
    ee.x = e->x;
    ee.y = height - 1 - e->y;
    sg_game_event((union sg_event *) &ee);
}

static void
sdl_event_mousebutton(SDL_MouseButtonEvent *e)
{
    struct sg_event_mouse ee;
    int width, height;
    SDL_GetWindowSize(sg_window, &width, &height);
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
    ee.y = height - 1 - e->y;
    sg_game_event((union sg_event *) &ee);
}

static void
sdl_event_key(SDL_KeyboardEvent *e)
{
    struct sg_event_key ee;
    ee.type = e->type == SDL_KEYDOWN ?
        SG_EVENT_KDOWN : SG_EVENT_KUP;
    /* SDL scancodes are just HID codes, which is what we use.  */
    ee.key = e->keysym.scancode;
    sg_game_event((union sg_event *) &ee);
}

static void
sdl_main(void)
{
    SDL_Event e;
    int width, height;
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

            default:
                break;
            }
        }

        SDL_GetWindowSize(sg_window, &width, &height);
        sg_game_draw(width, height, sg_clock_get());
        SDL_GL_SwapWindow(sg_window);
    }
}

int
main(int argc, char *argv[])
{
    sdl_init(argc, argv);
    sdl_main();
    return 0;
}
