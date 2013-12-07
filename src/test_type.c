/* Copyright 2013 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "defs.h"
#include "sg/opengl.h"
#include "sg/entry.h"
#include "sg/event.h"
#include "sg/type.h"
#include "keycode/keycode.h"
#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char TEXT1[] = "Text Demo";

static const char TEXT2[] = "吾輩は猫である";

static const char TEXT3[] =
"Solemnly he came forward and mounted the round gunrest. He faced about and "
"blessed gravely thrice the tower, the surrounding land and the awaking "
"mountains. Then, catching sight of Stephen Dedalus, he bent towards him "
"and made rapid crosses in the air, gurgling in his throat and shaking his "
"head. Stephen Dedalus, displeased and sleepy, leaned his arms on the top "
"of the staircase and looked coldly at the shaking gurgling face that "
"blessed him, equine in its length, and at the light untonsured hair, "
"grained and hued like pale oak.";

static const char TEXT4[] =
"1: invert, 2: boxes";

static struct text_layout g_text1, g_text2, g_text3, g_text4;
static int g_dark, g_boxes;
static struct prog_text g_prog_text;
static struct prog_plain g_prog_plain;
static GLuint g_bar_buffer;

static const short VERTEX[] = { -1, -1, 1, -1, -1, 1, 1, 1 };

static void
st_type_init(void)
{
    text_layout_create(&g_text1, TEXT1, strlen(TEXT1),
                       "Sans", 36.0, SG_TEXTALIGN_LEFT, 0.0);
    text_layout_create(&g_text2, TEXT2, strlen(TEXT2),
                       "Sans", 36.0, SG_TEXTALIGN_LEFT, 0.0);
    text_layout_create(&g_text3, TEXT3, strlen(TEXT3),
                       "Sans", 16.0, SG_TEXTALIGN_LEFT, 500.0);
    text_layout_create(&g_text4, TEXT4, strlen(TEXT4),
                       "Sans", 16.0, SG_TEXTALIGN_LEFT, 0.0);

    
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
st_type_set_layout(struct text_layout *layout)
{
    glBindTexture(GL_TEXTURE_2D, layout->texture);
    glUniform2fv(g_prog_text.u_texscale, 1, layout->texture_scale);
    glBindBuffer(GL_ARRAY_BUFFER, layout->buffer);
    glVertexAttribPointer(g_prog_text.a_vert, 4, GL_SHORT, GL_FALSE, 0, 0);
}

static void
st_type_draw_layout(struct text_layout *layout, int width, int height,
                    int x, int y, int xalign, int yalign)
{
    int px, py;

    switch (xalign) {
    default: abort();
    case 0:
        px = layout->metrics.logical.x0;
        break;

    case 1:
        px = (layout->metrics.logical.x0 + layout->metrics.logical.x1) >> 1;
        break;

    case 2:
        px = layout->metrics.logical.x1;
        break;
    }

    switch (yalign) {
    default: abort();
    case 0:
        py = layout->metrics.logical.y0;
        break;

    case 1:
        py = (layout->metrics.logical.y0 + layout->metrics.logical.y1) >> 1;
        break;

    case 2:
        py = layout->metrics.logical.y1;
        break;
    }

    glUniform2f(
        g_prog_text.u_vertoff,
        - 0.5f * (float)  width + (float) (x - px),
        - 0.5f * (float) height + (float) (y - py));
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

static void
st_type_draw(int x, int y, int width, int height, unsigned msec)
{
    float mod;

    glViewport(x, y, width, height);
    if (!g_dark)
        glClearColor(0.0, 0.0, 0.0, 0.0);
    else
        glClearColor(1.0, 1.0, 1.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    /* Bar */

    mod = (float) (msec & ((1u << 12) - 1)) / (1 << 12);
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

    st_type_set_layout(&g_text1);
    st_type_draw_layout(&g_text1, width, height, 10, 10, 0, 0);
    st_type_draw_layout(&g_text1, width, height, width - 10, 10, 2, 0);
    st_type_draw_layout(&g_text1, width, height, 10, height - 10, 0, 2);
    st_type_draw_layout(
        &g_text1, width, height, width - 10, height - 10, 2, 2);

    /* Test non-ASCII characters */

    st_type_set_layout(&g_text2);
    st_type_draw_layout(&g_text2, width, height, 10, height - 50, 0, 2);

    /* Test wrapping */

    st_type_set_layout(&g_text3);
    st_type_draw_layout(&g_text3, width, height, 10, height - 90, 0, 2);

    /* Instructions */

    st_type_set_layout(&g_text4);
    st_type_draw_layout(&g_text4, width, height, width/2, 10, 1, 0);

    /* Cleanup */

    glDisableVertexAttribArray(g_prog_text.a_vert);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
    glDisable(GL_BLEND);

    (void) msec;
}

const struct st_iface ST_TYPE = {
    "Type",
    st_type_init,
    st_type_destroy,
    st_type_event,
    st_type_draw
};
