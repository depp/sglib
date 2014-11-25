/* Copyright 2013-2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "defs.h"
#include "sg/opengl.h"
#include "sg/entry.h"
#include "sg/event.h"
#include "sg/type.h"
#include "sg/keycode.h"
#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *const TEXT[4] = {
"Text Demo",
"ζῷον δίπουν ἄπτερον", // two-legged featherless animal
"Solemnly he came forward and mounted the round gunrest. He faced about and "
"blessed gravely thrice the tower, the surrounding land and the awaking "
"mountains. Then, catching sight of Stephen Dedalus, he bent towards him "
"and made rapid crosses in the air, gurgling in his throat and shaking his "
"head. Stephen Dedalus, displeased and sleepy, leaned his arms on the top "
"of the staircase and looked coldly at the shaking gurgling face that "
"blessed him, equine in its length, and at the light untonsured hair, "
"grained and hued like pale oak.",
"1: invert, 2: boxes"
};

static struct sg_textlayout *g_text[4];
static int g_dark, g_boxes;
static struct prog_text g_prog_text;
static struct prog_plain g_prog_plain;
static GLuint g_bar_buffer;

static const short VERTEX[] = { -1, -1, 1, -1, -1, 1, 1, 1 };

static void
st_type_init(void)
{
    struct sg_typeface *typeface;
    struct sg_font *small, *large;
    struct sg_textflow *flow;
    struct sg_textlayout *layout;
    int i;
    const char *path = "font/Roboto-Regular";

    typeface = sg_typeface_file(path, strlen(path), NULL);
    if (!typeface) abort();
    small = sg_font_new(typeface, 16.0f, NULL);
    if (!small) abort();
    large = sg_font_new(typeface, 36.0f, NULL);
    if (!large) abort();
    sg_typeface_decref(typeface);

    for (i = 0; i < 4; i++) {
        flow = sg_textflow_new(NULL);
        if (!flow) abort();
        sg_textflow_setfont(flow, i < 2 ? large : small);
        if (i == 2)
            sg_textflow_setwidth(flow, 500.0f);
        sg_textflow_addtext(flow, TEXT[i], strlen(TEXT[i]));
        layout = sg_textlayout_new(flow, NULL);
        if (!layout) abort();
        sg_textflow_free(flow);
        g_text[i] = layout;
    }

    glGenBuffers(1, &g_bar_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, g_bar_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(VERTEX), VERTEX, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    load_prog_text(&g_prog_text);
    load_prog_plain(&g_prog_plain);
}

static void
st_type_destroy(void)
{ }

static void
st_type_event(union sg_event *evt)
{
    switch (evt->type) {
    case SG_EVENT_KDOWN:
        switch (evt->key.key) {
        case KEY_1:
            g_dark = !g_dark;
            break;
        case KEY_2:
            g_boxes = !g_boxes;
            break;
        }
        break;

    default:
        break;
    }
}

static void
st_type_draw_layout(struct sg_textlayout *layout, int width, int height,
                    int x, int y, int xalign, int yalign)
{
    int px, py;
    struct sg_textmetrics metrics;

    sg_textlayout_getmetrics(layout, &metrics);

    switch (xalign) {
    default: abort();
    case 0:
        px = metrics.logical.x0;
        break;

    case 1:
        px = (metrics.logical.x0 + metrics.logical.x1) >> 1;
        break;

    case 2:
        px = metrics.logical.x1;
        break;
    }

    switch (yalign) {
    default: abort();
    case 0:
        py = metrics.logical.y0;
        break;

    case 1:
        py = (metrics.logical.y0 + metrics.logical.y1) >> 1;
        break;

    case 2:
        py = metrics.logical.y1;
        break;
    }

    glUniform2f(
        g_prog_text.u_vertoff,
        - 0.5f * (float)  width + (float) (x - px),
        - 0.5f * (float) height + (float) (y - py));
    sg_textlayout_draw(layout, g_prog_text.a_vert, g_prog_text.u_texscale);
}

static void
st_type_draw(int width, int height, double time)
{
    float mod;

    glViewport(0, 0, width, height);
    if (!g_dark)
        glClearColor(0.0, 0.0, 0.0, 0.0);
    else
        glClearColor(1.0, 1.0, 1.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    /* Bar */

    mod = (float) fmod(time * 0.25, 1.0);
    mod = sinf((8 * atanf(1.0f)) * mod);

    glUseProgram(g_prog_plain.prog);
    glUniform2f(
        g_prog_plain.u_vertoff,
        mod * ((float) width * 0.5f - 32.0f) / 32.0f,
        0.0f);
    glUniform2f(g_prog_plain.u_vertscale, (float) (32.0 * 2.0 / width), 1.0f);
    if (g_dark)
        glUniform4f(g_prog_plain.u_color, 0.8f, 0.8f, 0.8f, 1.0f);
    else
        glUniform4f(g_prog_plain.u_color, 0.2f, 0.2f, 0.2f, 1.0f);
    glBindBuffer(GL_ARRAY_BUFFER, g_bar_buffer);
    glEnableVertexAttribArray(g_prog_plain.a_loc);
    glVertexAttribPointer(g_prog_plain.a_loc, 2, GL_SHORT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDisableVertexAttribArray(g_prog_plain.a_loc);

    /* Text */

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(g_prog_text.prog);
    glUniform2f(
        g_prog_text.u_vertscale,
        (float) (2.0 / width),
        (float) (2.0 / height));
    glUniform1i(g_prog_text.u_texture, 0);
    if (g_dark)
        glUniform4f(g_prog_text.u_color, 0.0f, 0.0f, 0.0f, 1.0f);
    else
        glUniform4f(g_prog_text.u_color, 1.0f, 1.0f, 1.0f, 0.0f);
    if (!g_boxes)
        glUniform4f(g_prog_text.u_bgcolor, 0.0f, 0.0f, 0.0f, 0.0f);
    else if (g_dark)
        glUniform4f(g_prog_text.u_bgcolor, 0.0f, 0.0f, 0.2f, 0.2f);
    else
        glUniform4f(g_prog_text.u_bgcolor, 0.2f, 0.2f, 0.0f, 0.2f);
    glActiveTexture(GL_TEXTURE0);
    glEnableVertexAttribArray(g_prog_text.a_vert);

    /* Test box alignment */

    st_type_draw_layout(g_text[0], width, height, 10, 10, 0, 0);
    st_type_draw_layout(g_text[0], width, height, width - 10, 10, 2, 0);
    st_type_draw_layout(g_text[0], width, height, 10, height - 10, 0, 2);
    st_type_draw_layout(
        g_text[0], width, height, width - 10, height - 10, 2, 2);

    /* Test non-ASCII characters */

    st_type_draw_layout(g_text[1], width, height, 10, height - 50, 0, 2);

    /* Test wrapping */

    st_type_draw_layout(g_text[2], width, height, 10, height - 90, 0, 2);

    /* Instructions */

    st_type_draw_layout(g_text[3], width, height, width/2, 10, 1, 0);

    /* Cleanup */

    glDisableVertexAttribArray(g_prog_text.a_vert);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
    glDisable(GL_BLEND);
}

const struct st_iface ST_TYPE = {
    "Type",
    st_type_init,
    st_type_destroy,
    st_type_event,
    st_type_draw
};
