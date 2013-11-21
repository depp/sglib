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

#define BOX_MARGIN 4
#define BOX_WIDTH 100
#define BOX_HEIGHT 18

static const struct st_iface *const ST_ITEMS[] = {
    &ST_IMAGE, &ST_TYPE, &ST_AUDIO
};
#define ST_NITEMS ((int) (sizeof(ST_ITEMS) / sizeof(*ST_ITEMS)))

enum {
    ST_MENU_BUTTON_DOWN,
    ST_MENU_BUTTON_UP
};

static int g_menu_item = 0;
static unsigned g_buttons = 0;
static struct sg_layout *g_menu_text[ST_NITEMS];
static GLuint g_box_buffer;

struct {
    GLuint prog;
    GLuint u_vertoff, u_vertscale, u_color;
    GLuint a_loc;
} g_prog_plain;

static void st_menu_load_plainprog(void)
{
    GLuint prog;
    g_prog_plain.prog = prog = load_program(
        "shader/plain.vert", "shader/plain.frag");
#define UNIFORM(x) g_prog_plain.x = glGetUniformLocation(prog, #x)
    UNIFORM(u_vertoff);
    UNIFORM(u_vertscale);
    UNIFORM(u_color);
#undef UNIFORM
    g_prog_plain.a_loc = glGetAttribLocation(prog, "a_loc");
}

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

    st_menu_load_plainprog();

    static const short VERTEX[] = {
        -BOX_MARGIN, -BOX_MARGIN,
        BOX_MARGIN + BOX_WIDTH, -BOX_MARGIN,
        BOX_MARGIN + BOX_WIDTH, BOX_MARGIN + BOX_HEIGHT,
        -BOX_MARGIN, BOX_MARGIN + BOX_HEIGHT
    };
    glGenBuffers(1, &g_box_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, g_box_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(VERTEX), VERTEX, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
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
    float xscale, yscale;
    int i;
    sg_opengl_checkerror("st_menu_draw start");

    xscale = 2.0f / (float) width;
    yscale = 2.0f / (float) height;

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
    glUseProgram(g_prog_plain.prog);
    glUniform2f(
        g_prog_plain.u_vertoff,
        32.0f - 0.5f * (float) width,
        0.5f * (float) height - 32.0f * (float) (i + 1) - BOX_HEIGHT);
    glUniform2f(g_prog_plain.u_vertscale, xscale, yscale);
    glUniform4f(g_prog_plain.u_color, 1.0f, 1.0f, 1.0f, 1.0f);
    glBindBuffer(GL_ARRAY_BUFFER, g_box_buffer);
    glEnableVertexAttribArray(g_prog_plain.a_loc);
    glVertexAttribPointer(g_prog_plain.a_loc, 2, GL_SHORT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDrawArrays(GL_LINE_LOOP, 0, 4);
    glDisableVertexAttribArray(g_prog_plain.a_loc);
    glUseProgram(0);

    sg_opengl_checkerror("st_menu_draw");
    (void) msec;
}

const struct st_iface ST_MENU = {
    NULL,
    st_menu_init,
    st_menu_destroy,
    st_menu_event,
    st_menu_draw
};
