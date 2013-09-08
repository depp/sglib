/* Copyright 2013 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "defs.h"
#include "keycode/keycode.h"
#include "sg/event.h"
#include "sg/opengl.h"
#include "sg/type.h"
#include <assert.h>
#include <string.h>

#define BOX_MARGIN 4.0f
#define BOX_WIDTH 100.0f
#define BOX_HEIGHT 18.0f

static const struct st_iface *const ST_ITEMS[] = {
    &ST_IMAGE, &ST_TYPE, &ST_AUDIO, &ST_AUDIO2
};
#define ST_NITEMS ((int) (sizeof(ST_ITEMS) / sizeof(*ST_ITEMS)))

enum {
    ST_MENU_BUTTON_DOWN,
    ST_MENU_BUTTON_UP
};

static int g_menu_item = 0;
static unsigned g_buttons = 0;
static struct sg_layout *g_menu_text[ST_NITEMS];

static void st_menu_init(void)
{
    int i;
    struct sg_layout *lp;
    for (i = 0; i < ST_NITEMS; i++) {
        lp = sg_layout_new();
        assert(lp);
        sg_layout_settext(lp, ST_ITEMS[i]->name,
                          (unsigned) strlen(ST_ITEMS[i]->name));
        sg_layout_setboxalign(lp, SG_VALIGN_TOP | SG_HALIGN_LEFT);
        g_menu_text[i] = lp;
    }
    g_buttons = 0;
}

static void st_menu_destroy(void)
{
    int i;
    for (i = 0; i < ST_NITEMS; i++) {
        if (!g_menu_text[i])
            continue;
        sg_layout_decref(g_menu_text[i]);
        g_menu_text[i] = NULL;
    }
}

static void st_menu_event(union sg_event *evt)
{
    int button;
    switch (evt->type) {
    case SG_EVENT_KDOWN:
    case SG_EVENT_KUP:
        switch (evt->key.key) {
        case KEY_Down:
            button = ST_MENU_BUTTON_DOWN;
            break;

        case KEY_Up:
            button = ST_MENU_BUTTON_UP;
            break;

        case KEY_Enter:
            st_menu_destroy();
            st_screen = ST_ITEMS[g_menu_item];
            st_screen->init();
            return;

        default:
            button = -1;
            break;
        }
        if (button >= 0) {
            if (evt->type == SG_EVENT_KDOWN) {
                if ((g_buttons & (1u << button)) == 0) {
                    switch (button) {
                    case ST_MENU_BUTTON_DOWN:
                        if (g_menu_item < ST_NITEMS - 1)
                            g_menu_item++;
                        break;

                    case ST_MENU_BUTTON_UP:
                        if (g_menu_item > 0)
                            g_menu_item--;
                        break;

                    default:
                        break;
                    }
                    g_buttons |= 1u << button;
                }
            } else {
                g_buttons &= ~(1u << button);
            }
        }
        break;

    default:
        break;
    }
}

static void st_menu_draw(int x, int y, int width, int height, unsigned msec)
{
    int i;

    glViewport(x, y, width, height);
    glClearColor(0.1, 0.2, 0.3, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, (double) width, 0, (double) height, 1.0, -1.0);
    glMatrixMode(GL_MODELVIEW);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    for (i = 0; i < ST_NITEMS; i++) {
        glPushMatrix();
        glTranslatef(32.0f, (float) height - 32.0f * (float) (i + 1), 0.0f);
        sg_layout_draw(g_menu_text[i]);
        glPopMatrix();
    }

    glDisable(GL_BLEND);

    i = g_menu_item;
    glPushMatrix();
    glTranslatef(32.0f, (float) height - 32.0f * (float) (i + 1), 0.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(-BOX_MARGIN, BOX_MARGIN);
    glVertex2f(BOX_WIDTH + BOX_MARGIN, BOX_MARGIN);
    glVertex2f(BOX_WIDTH + BOX_MARGIN, -BOX_HEIGHT - BOX_MARGIN);
    glVertex2f(-BOX_MARGIN, -BOX_HEIGHT - BOX_MARGIN);
    glEnd();
    glPopMatrix();

    (void) msec;
}

const struct st_iface ST_MENU = {
    NULL,
    st_menu_init,
    st_menu_destroy,
    st_menu_event,
    st_menu_draw
};
