/* Copyright 2013-2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "defs.h"
#include "sg/keycode.h"
#include "sg/entry.h"
#include "sg/event.h"
#include "sg/mixer.h"
#include "sg/record.h"

const struct st_iface *st_screen;

void
sg_game_init(void)
{
    st_screen = &ST_MENU;
    sg_mixer_start();
}

void
sg_game_destroy(void)
{ }

void
sg_game_getinfo(struct sg_game_info *info)
{
    (void) info;
}

void
sg_game_event(union sg_event *evt)
{
    static int escape_down;
    switch (evt->common.type) {
    case SG_EVENT_VIDEO_INIT:
        st_screen->init();
        return;

    case SG_EVENT_VIDEO_TERM:
        return;

    case SG_EVENT_KDOWN:
        if (evt->key.key == KEY_Escape) {
            if (!escape_down) {
                if (st_screen == &ST_MENU) {
                    sg_sys_quit();
                } else if (st_screen) {
                    st_screen->destroy();
                    st_screen = &ST_MENU;
                    st_screen->init();
                }
                escape_down = 1;
            }
            return;
        } else if (evt->key.key == KEY_Backslash) {
            sg_record_screenshot();
        }
        break;

    case SG_EVENT_KUP:
        if (evt->key.key == KEY_Escape) {
            escape_down = 0;
            return;
        }
        break;

    default:
        break;
    }

    st_screen->event(evt);
}

void
sg_game_draw(int width, int height, double time)
{
    sg_mixer_settime(time);
    st_screen->draw(width, height, time);
    sg_mixer_commit();
}
