/* Copyright 2012-2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "sg/clock.h"
#include "sg/cvar.h"
#include "sg/defs.h"
#include "sg/entry.h"
#include "sg/error.h"
#include "sg/event.h"
#include "sg/log.h"
#include "sg/opengl.h"
#include "sg/version.h"
#include "../clock_impl.h"
#include "../private.h"
#include "SDL.h"
#include <stdio.h>

#include "sg/key.h"

struct sg_sdl {
    SDL_Window *window;
    SDL_GLContext context;
    int window_status;
    int want_capture;
    int have_capture;
    struct sg_cvar_bool enable_hidpi;
};

static struct sg_sdl sg_sdl;

SG_ATTR_NORETURN
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

static void
sg_sdl_updatecapture(void)
{
    int enabled = sg_sdl.want_capture &&
        (sg_sdl.window_status & SG_WINDOW_FOCUSED) != 0;
    int r = SDL_SetRelativeMouseMode(enabled ? SDL_TRUE : SDL_FALSE);
    if (r == 0) {
        sg_sdl.have_capture = enabled;
    } else {
        if (enabled)
            sg_logs(SG_LOG_WARN, "failed to set relative mouse mode");
        return;
    }
}

void
sg_sys_capturemouse(int enabled)
{
    sg_sdl.want_capture = enabled;
    sg_sdl_updatecapture();
}

void
sg_version_platform(void)
{
    char v1[16], v2[16];
    SDL_version v;
    SDL_GetVersion(&v);
#if !defined _WIN32
    snprintf(v1, sizeof(v1), "%d.%d.%d",
             SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL);
    snprintf(v2, sizeof(v2), "%d.%d.%d",
             v.major, v.minor, v.patch);
#else
    _snprintf_s(v1, sizeof(v1), _TRUNCATE, "%d.%d.%d",
        SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL);
    _snprintf_s(v2, sizeof(v2), _TRUNCATE, "%d.%d.%d",
        v.major, v.minor, v.patch);
#endif
    sg_version_lib("LibSDL", v1, v2);
}

SG_ATTR_NORETURN
static void
sdl_error(const char *what)
{
    fprintf(stderr, "error: %s: %s\n", what, SDL_GetError());
    sdl_quit(1);
}

static void
sg_sdl_setvsync(void)
{
    int vsync = sg_sys.vsync.value, r;
    sg_sys.vsync.flags &= ~SG_CVAR_MODIFIED;
    switch (vsync) {
    case 0:
        r = SDL_GL_SetSwapInterval(0);
        break;
    case 1:
        r = SDL_GL_SetSwapInterval(1);
        break;
    case 2:
        r = SDL_GL_SetSwapInterval(-1);
        if (!r)
            return;
        sg_logf(SG_LOG_WARN, "Could not set vsync 2: %s", SDL_GetError());
        SDL_ClearError();
        vsync = 1;
        r = SDL_GL_SetSwapInterval(1);
        break;
    default:
        return;
    }
    if (r) {
        sg_logf(SG_LOG_WARN, "Could not set vsync %d: %s",
                vsync, SDL_GetError());
    }
}

static void
sdl_init(int argc, char *argv[])
{
    GLenum err;
    union sg_event evt;
    struct sg_game_info gameinfo;
    unsigned flags;
    int width, height, r;

    flags = SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS;
    if (SDL_Init(flags))
        sdl_error("could not initialize LibSDL");

    sg_sys_init(&gameinfo, argc - 1, argv + 1);

    flags = SDL_WINDOW_OPENGL;
    if (sg_sys.hidpi.value &&
        (gameinfo.flags & SG_GAME_ALLOW_HIDPI) != 0) {
        flags |= SDL_WINDOW_ALLOW_HIGHDPI;
    }
    if ((gameinfo.flags & SG_GAME_ALLOW_RESIZE) != 0) {
        flags |= SDL_WINDOW_RESIZABLE;
    }
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(
        SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    sg_sys.vidsize.flags &= ~SG_CVAR_MODIFIED;
    r = sg_sys_getvidsize(&width, &height);
    if (r) {
        width = gameinfo.default_width;
        height = gameinfo.default_height;
    }
    sg_sdl.window = SDL_CreateWindow(
        gameinfo.name,
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        width, height, flags);
    if (!sg_sdl.window)
        sdl_error("could not open window");

    sg_sdl.context = SDL_GL_CreateContext(sg_sdl.window);
    if (!sg_sdl.context)
        sdl_error("could not create OpenGL context");

    /* Apparently, GLEW queries extensions with glGetString(GL_EXTENSIONS),
       which is incorrect for the OpenGL core profile, and glGetStringi
       should instead be called on each extension.  This is a workaround.  */
    glewExperimental = GL_TRUE;
    err = glewInit();
    if (err)
        sg_sys_abort("could not initialize GLEW");
    sg_opengl_checkerror("glewInit");

    sg_sdl_setvsync();

    evt.common.time = sg_clock_get();
    evt.common.type = SG_EVENT_VIDEO_INIT;
    sg_game_event(&evt);
}

static void
sdl_event_mousemove(SDL_MouseMotionEvent *e)
{
    union sg_event ee;
    int width, height;
    ee.mouse.time = sg_clock_convert_sdl(e->timestamp);
    if (sg_sdl.have_capture) {
        ee.mouse.type = SG_EVENT_MMOVEREL;
        ee.mouse.button = -1;
        ee.mouse.x = e->xrel;
        ee.mouse.y = -e->yrel;
    } else {
        SDL_GetWindowSize(sg_sdl.window, &width, &height);
        ee.mouse.type = SG_EVENT_MMOVE;
        ee.mouse.button = -1;
        ee.mouse.x = e->x;
        ee.mouse.y = height - 1 - e->y;
    }
    sg_game_event(&ee);
}

static void
sdl_event_mousebutton(SDL_MouseButtonEvent *e)
{
    union sg_event ee;
    int width, height;
    SDL_GetWindowSize(sg_sdl.window, &width, &height);
    ee.mouse.time = sg_clock_convert_sdl(e->timestamp);
    ee.mouse.type = e->type == SDL_MOUSEBUTTONDOWN ?
        SG_EVENT_MDOWN : SG_EVENT_MUP;
    switch (e->button) {
    case SDL_BUTTON_LEFT:   ee.mouse.button = SG_BUTTON_LEFT;   break;
    case SDL_BUTTON_MIDDLE: ee.mouse.button = SG_BUTTON_MIDDLE; break;
    case SDL_BUTTON_RIGHT:  ee.mouse.button = SG_BUTTON_RIGHT;  break;
    default:
        ee.mouse.button = SG_BUTTON_OTHER + e->button - 4;
        break;
    }
    ee.mouse.x = e->x;
    ee.mouse.y = height - 1 - e->y;
    sg_game_event(&ee);
}

static void
sdl_event_key(SDL_KeyboardEvent *e)
{
    union sg_event ee;
    ee.key.time = sg_clock_convert_sdl(e->timestamp);
    ee.key.type = e->type == SDL_KEYDOWN ?
        SG_EVENT_KDOWN : SG_EVENT_KUP;
    /* SDL scancodes are just HID codes, which is what we use.  */
    ee.key.key = e->keysym.scancode;
    sg_game_event(&ee);
}

#if 0

struct sdl_event_wtype {
    short type;
    char name[14];
};

#define TYPE(x) { SDL_WINDOWEVENT_ ## x, #x }
static const struct sdl_event_wtype SDL_EVENT_WTYPE[] = {
    TYPE(SHOWN),
    TYPE(HIDDEN),
    TYPE(EXPOSED),
    TYPE(MOVED),
    TYPE(RESIZED),
    TYPE(MINIMIZED),
    TYPE(MAXIMIZED),
    TYPE(RESTORED),
    TYPE(ENTER),
    TYPE(LEAVE),
    TYPE(FOCUS_GAINED),
    TYPE(FOCUS_LOST),
    TYPE(CLOSE)
};
#undef TYPE

static void
sdl_event_wtype_showname(int wtype)
{
    const struct sdl_event_wtype *tp = SDL_EVENT_WTYPE,
        *te = tp + sizeof(SDL_EVENT_WTYPE) / sizeof(*SDL_EVENT_WTYPE);
    for (; tp != te; tp++) {
        if (tp->type == wtype) {
            sg_logf(SG_LOG_DEBUG,
                    "Window event: %s", tp->name);
            return;
        }
    }
    sg_logf(SG_LOG_DEBUG,
            "Window event: unknown (%d)", wtype);
}

#else

#define sdl_event_wtype_showname(x) (void) 0

#endif

static void
sdl_event_window(SDL_WindowEvent *e)
{
    union sg_event ee;
    unsigned oldstatus, newstatus;
    sdl_event_wtype_showname(e->event);
    oldstatus = newstatus = sg_sdl.window_status;

    switch (e->event) {
    case SDL_WINDOWEVENT_SHOWN:
        newstatus |= SG_WINDOW_VISIBLE;
        break;

    case SDL_WINDOWEVENT_HIDDEN:
        newstatus &= ~SG_WINDOW_VISIBLE;
        break;

    case SDL_WINDOWEVENT_FOCUS_GAINED:
        newstatus |= SG_WINDOW_FOCUSED;
        break;

    case SDL_WINDOWEVENT_FOCUS_LOST:
        newstatus &= ~SG_WINDOW_FOCUSED;
        break;

    default:
        break;
    }

    if (oldstatus == newstatus)
        return;

    sg_sdl.window_status = newstatus;
    sg_sdl_updatecapture();
    ee.status.time = sg_clock_convert_sdl(e->timestamp);
    ee.status.type = SG_EVENT_WINDOW;
    ee.status.status = newstatus;
    sg_game_event(&ee);
}

static void
sdl_main(void)
{
    SDL_Event e;
    int width, height;
    int last_time = SDL_GetTicks(), new_time, maxfps;
    while (1) {
        SDL_PumpEvents();
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
            case SDL_QUIT:
                return;

            case SDL_MOUSEMOTION:
                // printf("MOUSEMOVE\n");
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

            case SDL_WINDOWEVENT:
                sdl_event_window(&e.window);
                break;

            default:
                // printf("Unknown event: %u\n", e.type);
                break;
            }
        }

        if ((sg_sys.vidsize.flags & SG_CVAR_MODIFIED) != 0) {
            int nwidth, nheight;
            sg_sys.vidsize.flags &= ~SG_CVAR_MODIFIED;
            sg_sys_getvidsize(&nwidth, &nheight);
            SDL_SetWindowSize(sg_sdl.window, nwidth, nheight);
        }

        SDL_GL_GetDrawableSize(sg_sdl.window, &width, &height);
        sg_sys_draw(width, height, sg_clock_get());
        SDL_GL_SwapWindow(sg_sdl.window);
        sg_sys_postdraw();

        new_time = SDL_GetTicks();
        maxfps = sg_sys.maxfps.value;
        if (maxfps > 0 && (new_time - last_time) * maxfps < 1000) {
            int delay;
            delay = 1000 / maxfps - (new_time - last_time);
            if (delay > 0)
                SDL_Delay(delay);
        }
        last_time = new_time;
    }
}

int
main(int argc, char *argv[])
{
    sdl_init(argc, argv);
    sdl_main();
    SDL_DestroyWindow(sg_sdl.window);
    sg_game_destroy();
    return 0;
}
