/* Copyright 2013 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "defs.h"
#include "sg/keycode.h"
#include "sg/event.h"
#include "sg/opengl.h"
#include "sg/type.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define BOX_MARGIN 4
#define BOX_WIDTH 100
#define BOX_HEIGHT 18

static const struct st_iface *const ST_ITEMS[] = {
    &ST_IMAGE,
    /* &ST_TYPE, */
    &ST_AUDIO
};
#define ST_NITEMS ((int) (sizeof(ST_ITEMS) / sizeof(*ST_ITEMS)))

enum {
    ST_MENU_BUTTON_DOWN,
    ST_MENU_BUTTON_UP
};

static int g_menu_item = 0;
static unsigned g_buttons = 0;
static struct sg_textlayout *g_menu_text[ST_NITEMS];
static GLuint g_box_buffer;
static struct prog_plain g_prog_plain;
static struct prog_text g_prog_text;

static const short VERTEX[] = {
    -BOX_MARGIN, -BOX_MARGIN,
    BOX_MARGIN + BOX_WIDTH, -BOX_MARGIN,
    BOX_MARGIN + BOX_WIDTH, BOX_MARGIN + BOX_HEIGHT,
    -BOX_MARGIN, BOX_MARGIN + BOX_HEIGHT
};

static void st_menu_init(void)
{
    int i;
    struct sg_typeface *typeface;
    struct sg_font *font;
    struct sg_textflow *flow;
    const char *path;

    sg_opengl_checkerror("st_menu_init start");

    path = "font/Roboto-Regular";
    typeface = sg_typeface_file(path, strlen(path), NULL);
    if (!typeface)
        abort();
    font = sg_font_new(typeface, 12.0f, NULL);
    if (!font)
        abort();
    sg_typeface_decref(typeface);

    for (i = 0; i < ST_NITEMS; i++) {
        flow = sg_textflow_new(NULL);
        if (!flow)
            abort();
        sg_textflow_setfont(flow, font);
        sg_textflow_addtext(
            flow, ST_ITEMS[i]->name, strlen(ST_ITEMS[i]->name));
        g_menu_text[i] = sg_textlayout_new(flow, NULL);
        if (!g_menu_text[i])
            abort();
        sg_textflow_free(flow);
    }
    g_buttons = 0;

    glGenBuffers(1, &g_box_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, g_box_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(VERTEX), VERTEX, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    sg_opengl_checkerror("st_menu_init");

    load_prog_plain(&g_prog_plain);
    load_prog_text(&g_prog_text);
}

static void st_menu_destroy(void)
{
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

static void st_menu_draw(int width, int height, unsigned msec)
{
    struct sg_textmetrics metrics;
    float xscale, yscale;
    int i;
    sg_opengl_checkerror("st_menu_draw start");

    xscale = 2.0f / (float) width;
    yscale = 2.0f / (float) height;

    glViewport(0, 0, width, height);
    glClearColor(0.1, 0.2, 0.3, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(g_prog_text.prog);
    glUniform2f(
        g_prog_text.u_vertscale, xscale, yscale);
    glUniform1i(g_prog_text.u_texture, 0);
    glUniform4f(g_prog_text.u_color, 1.0f, 1.0f, 1.0f, 1.0f);
    glUniform4f(g_prog_text.u_bgcolor, 0.0f, 0.0f, 0.0f, 0.0f);
    glActiveTexture(GL_TEXTURE0);
    glEnableVertexAttribArray(g_prog_text.a_vert);

    for (i = 0; i < ST_NITEMS; i++) {
        sg_textlayout_getmetrics(g_menu_text[i], &metrics);
        glUniform2f(
            g_prog_text.u_vertoff,
            -0.5f * (float) width + 32.0f
            - metrics.logical.x0,
            +0.5f * (float) height - 32.0f * (float) (i + 1)
            - metrics.logical.y1);
        sg_textlayout_draw(g_menu_text[i],
                           g_prog_text.a_vert, g_prog_text.u_texscale);
    }

    glEnableVertexAttribArray(g_prog_text.a_vert);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
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
