/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "base/cvar.h"
#include "base/entry.h"
#include "base/error.h"
#include "base/event.h"
#include "base/version.h"
#include "SDL.h"
#include <getopt.h>
#include <stdio.h>

static int sg_window_width, sg_window_height;

__attribute__((noreturn))
static void
quit(int status)
{
    SDL_Quit();
    exit(status);
}

void
sg_platform_quit(void)
{
    quit(0);
}

void
sg_platform_failf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    sg_platform_failv(fmt, ap);
    va_end(ap);
}

void
sg_platform_failv(const char *fmt, va_list ap)
{
    vfprintf(stderr, fmt, ap);
    quit(1);
}

void
sg_platform_faile(struct sg_error *err)
{
    if (err) {
        if (err->code)
            fprintf(stderr, "error: %s (%s %ld)\n",
                    err->msg, err->domain->name, err->code);
        else
            fprintf(stderr, "error: %s (%s)\n",
                    err->msg, err->domain->name);
    } else {
        fputs("error: an unknown error occurred\n", stderr);
    }
    quit(1);
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
    quit(1);
}

static SDL_Surface *sdl_surface;

static void
init(int argc, char *argv[])
{
    struct sg_event_resize rsz;
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
    sg_window_width = sdl_surface->w;
    sg_window_height = sdl_surface->h;
    rsz.type = SG_EVENT_RESIZE;
    rsz.width = sdl_surface->w;
    rsz.height = sdl_surface->h;
    sg_sys_event((union sg_event *) &rsz);
}

static void
sdl_event_mousemove(SDL_MouseMotionEvent *e)
{
    struct sg_event_mouse ee;
    ee.type = SG_EVENT_MMOVE;
    ee.button = -1;
    ee.x = e->x;
    ee.y = sg_window_height - 1 - e->y;
    sg_sys_event((union sg_event *) &ee);
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
    sg_sys_event((union sg_event *) &ee);
}

static void
sdl_event_key(SDL_KeyboardEvent *e)
{
    struct sg_event_key ee;
    ee.type = e->type == SDL_KEYDOWN ?
        SG_EVENT_KDOWN : SG_EVENT_KUP;
    ee.key = -1;
    sg_sys_event((union sg_event *) &ee);
}

static void
sdl_event_resize(SDL_ResizeEvent *e)
{
    struct sg_event_resize ee;
    sg_window_width = e->w;
    sg_window_height = e->h;
    ee.type = SG_EVENT_RESIZE;
    ee.width = e->w;
    ee.height = e->h;
    sg_sys_event((union sg_event *) &ee);
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

        sg_sys_draw();
        SDL_GL_SwapBuffers();
    }
}

int
main(int argc, char *argv[])
{
    init(argc, argv);
    sdl_main();
    return 0;
}