#include "client/viewport.hpp"
#include "event.hpp"
#include "impl/entry.h"
#include "impl/event.h"
#include "impl/opengl.h"
#include "impl/resource.h"
#include "menu.hpp"
#include "screen.hpp"
#include "sys/error.hpp"
#include <stdio.h>
using namespace UI;

static Screen *current_screen;

Screen::~Screen()
{ }

void Screen::init()
{ }

void Screen::makeActive(Screen *s)
{
    if (!s)
        throw std::invalid_argument("NULL screen");
    if (s != current_screen) {
        delete current_screen;
        current_screen = s;
    }
}

void Screen::quit()
{
    sg_platform_quit();
}

static void handle_error(error &ex)
{
    sg_platform_faile(ex.get());
}

static void handle_exception(std::exception &ex)
{
    sg_platform_failf("exception caught: %s", ex.what());
}

static void handle_nullscreen()
{
    sg_platform_failf("current screen is NULL");
}

void sg_game_init(void)
{
    current_screen = new UI::Menu;
}

void sg_game_getinfo(struct sg_game_info *info)
{
    (void) info;
}

static void mouse_event(union sg_event *evt, EventType t)
{
    struct sg_event_mouse &me = evt->mouse;
    MouseEvent ue(t, me.button, me.x, me.y);
    current_screen->handleEvent(ue);
}

static void key_event(union sg_event *evt, EventType t)
{
    struct sg_event_key &ke = evt->key;
    KeyEvent ue(t, ke.key);
    current_screen->handleEvent(ue);
}

void sg_game_event(union sg_event *evt)
{
    if (!current_screen) {
        handle_nullscreen();
        return;
    }
    try {
        switch (evt->type) {
        case SG_EVENT_MDOWN:
            mouse_event(evt, MouseDown);
            break;

        case SG_EVENT_MUP:
            mouse_event(evt, MouseUp);
            break;

        case SG_EVENT_MMOVE:
            mouse_event(evt, MouseMove);
            break;

        case SG_EVENT_KDOWN:
            key_event(evt, KeyDown);
            break;

        case SG_EVENT_KUP:
            key_event(evt, KeyUp);
            break;

        case SG_EVENT_RESIZE:
            break;
        }
    } catch (error &err) {
        handle_error(err);
    } catch (std::exception &ex) {
        handle_exception(ex);
    }
}

void sg_game_draw(int x, int y, int width, int height, unsigned msec)
{
    if (!current_screen) {
        handle_nullscreen();
        return;
    }
    try {
        current_screen->update(msec);
        sg_resource_loadall();
        {
            Viewport v(x, y, width, height);
            current_screen->draw(v, msec);
        }
        GLenum err = glGetError();
        if (err != GL_NO_ERROR) {
            const GLubyte *s = gluErrorString(err);
            fprintf(stderr, "GL Error: %s\n", s);
        }
    } catch (error &err) {
        handle_error(err);
    } catch (std::exception &ex) {
        handle_exception(ex);
    }
}

void sg_game_destroy(void)
{
    if (current_screen) {
        delete current_screen;
        current_screen = NULL;
    }
}
