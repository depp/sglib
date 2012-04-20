#include "base/opengl.h"
#include "base/entry.h"
#include "base/event.h"
#include "base/keycode/keycode.h"
#include "base/type.h"
#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
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

static struct sg_layout *g_text1, *g_text2, *g_text3, *g_text4;

static int g_dark, g_boxes;

void
sg_game_init(void)
{
    int a = SG_VALIGN_TOP | SG_HALIGN_LEFT;
    struct sg_style *sp;

    sp = sg_style_new();
    sg_style_setsize(sp, 36.0f);

    g_text1 = sg_layout_new();
    assert(g_text1);
    sg_layout_settext(g_text1, TEXT1, (unsigned) strlen(TEXT1));
    sg_layout_setstyle(g_text1, sp);
    sg_layout_setboxalign(g_text1, a);

    g_text2 = sg_layout_new();
    assert(g_text2);
    sg_layout_settext(g_text2, TEXT2, (unsigned) strlen(TEXT2));
    sg_layout_setstyle(g_text2, sp);
    sg_layout_setboxalign(g_text2, a);

    sg_style_setsize(sp, 16.0f);

    g_text3 = sg_layout_new();
    assert(g_text3);
    sg_layout_settext(g_text3, TEXT3, (unsigned) strlen(TEXT3));
    sg_layout_setstyle(g_text3, sp);
    sg_layout_setwidth(g_text3, 500);
    sg_layout_setboxalign(g_text3, a);

    g_text4 = sg_layout_new();
    assert(g_text4);
    sg_layout_settext(g_text4, TEXT4, (unsigned) strlen(TEXT4));
    sg_layout_setstyle(g_text4, sp);
    sg_layout_setboxalign(g_text4, SG_VALIGN_BOTTOM | SG_HALIGN_CENTER);

    sg_style_decref(sp);
}

void
sg_game_getinfo(struct sg_game_info *info)
{
    (void) info;
}

void
sg_game_event(union sg_event *evt)
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

void
sg_game_draw(int x, int y, int width, int height, unsigned msec)
{
    float x0, x1, y0, y1;
    float mod;

    if (!g_dark)
        glClearColor(0.0, 0.0, 0.0, 0.0);
    else
        glClearColor(1.0, 1.0, 1.0, 1.0);

    glClear(GL_COLOR_BUFFER_BIT);
    glViewport(x, y, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, (double) width, 0, (double) height, 1.0, -1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    mod = (float) (msec & ((1u << 12) - 1)) / (1 << 12);
    mod = (1.0f + sinf((8 * atanf(1.0f)) * mod)) * 0.5f * (width - 64);

    x0 = mod; x1 = mod + 64.0f;
    y0 = 0; y1 = (float) height;
    if (!g_dark)
        glColor3ub(51, 51, 51);
    else
        glColor3ub(204, 204, 204);
    glBegin(GL_TRIANGLE_STRIP);
    glVertex2f(x0, y0);
    glVertex2f(x1, y0);
    glVertex2f(x0, y1);
    glVertex2f(x1, y1);
    glEnd();

    if (!g_dark)
        glColor3ub(255, 255, 255);
    else
        glColor3ub(0, 0, 0);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    /* Test box alignment */

    glPushMatrix();
    glTranslatef(10, (float) height - 10, 0);
    sg_layout_setboxalign(g_text1, SG_VALIGN_TOP | SG_HALIGN_LEFT);
    sg_layout_draw(g_text1);
    if (g_boxes) sg_layout_drawmarks(g_text1);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(10, 10, 0);
    sg_layout_setboxalign(g_text1, SG_VALIGN_BOTTOM | SG_HALIGN_LEFT);
    sg_layout_draw(g_text1);
    if (g_boxes) sg_layout_drawmarks(g_text1);
    glPopMatrix();

    glPushMatrix();
    glTranslatef((float) width - 10, (float) height - 10, 0);
    sg_layout_setboxalign(g_text1, SG_VALIGN_TOP | SG_HALIGN_RIGHT);
    sg_layout_draw(g_text1);
    if (g_boxes) sg_layout_drawmarks(g_text1);
    glPopMatrix();

    glPushMatrix();
    glTranslatef((float) width - 10, 10, 0);
    sg_layout_setboxalign(g_text1, SG_VALIGN_BOTTOM | SG_HALIGN_RIGHT);
    sg_layout_draw(g_text1);
    if (g_boxes) sg_layout_drawmarks(g_text1);
    glPopMatrix();

    /* Test non-ASCII characters */

    glPushMatrix();
    glTranslatef(10, (float) height - 50, 0);
    sg_layout_draw(g_text2);
    if (g_boxes) sg_layout_drawmarks(g_text2);
    glPopMatrix();

    /* Test wrapping */

    glPushMatrix();
    glTranslatef(10, (float) height - 90, 0);
    sg_layout_draw(g_text3);
    if (g_boxes) sg_layout_drawmarks(g_text3);
    glPopMatrix();

    /* Instructions */

    glPushMatrix();
    glTranslatef((float) (width / 2), 10, 0);
    sg_layout_draw(g_text4);
    if (g_boxes) sg_layout_drawmarks(g_text4);
    glPopMatrix();

    (void) msec;
}

void
sg_game_destroy(void)
{ }
