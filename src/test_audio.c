/* Copyright 2013 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "defs.h"
#include "sg/audio_pcm.h"
#include "sg/audio_source.h"
#include "sg/entry.h"
#include "sg/opengl.h"
#include "sg/rand.h"
#include <assert.h>
#include <math.h>
#include <string.h>

#define NUMOBJS 10

static struct sg_audio_pcm_obj *g_snd_clank[3];
static struct sg_audio_pcm_obj *g_snd_donk[3];
static struct sg_audio_pcm_obj *g_snd_tink[3];
static int g_snd_source;

struct obj {
    float x, y;
    float dx, dy;
    float size;
    int type;
};

static int g_initted;
static unsigned g_tick;
static struct obj g_objs[NUMOBJS];

static void
st_audio_init(void)
{
    g_snd_clank[0] = load_audio("fx/clank1");
    g_snd_clank[1] = load_audio("fx/clank2");
    g_snd_clank[2] = load_audio("fx/clank3");
    g_snd_donk[0]  = load_audio("fx/donk1");
    g_snd_donk[1]  = load_audio("fx/donk2");
    g_snd_donk[2]  = load_audio("fx/donk3");
    g_snd_tink[0]  = load_audio("fx/tink1");
    g_snd_tink[1]  = load_audio("fx/tink2");
    g_snd_tink[2]  = load_audio("fx/tink3");
    g_snd_source = sg_audio_source_open();
    assert(g_snd_source >= 0);
}

static void
st_audio_destroy(void)
{ }

static void
st_audio_event(union sg_event *evt)
{
    (void) evt;
}

static void
st_audio_draw(int x, int y, int width, int height, unsigned msec)
{
    int i, j, sidx;
    float r, a, px, py, dx, dy;
    struct sg_audio_pcm_obj *snd;

    glClear(GL_COLOR_BUFFER_BIT);
    glViewport(x, y, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    if (width > height) {
        a = ((float) width / (float) height - 1.0f) * 0.5f;
        glOrtho(-a, 1.0 + a, 0, 1.0, 1.0, -1.0);
    } else {
        a = ((float) height / (float) width - 1.0f) * 0.5f;
        glOrtho(0, 1.0, -a, 1.0 + a, 1.0, -1.0);
    }
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    if (!g_initted) {
        g_initted = 1;
        g_tick = msec;

        for (i = 0; i < NUMOBJS / 3; ++i) {
            g_objs[i].size = 0.01;
            g_objs[i].dx = 0.10;
            g_objs[i].type = 0;
        }
        for (; i < (2 * NUMOBJS / 3); ++i) {
            g_objs[i].size = 0.05;
            g_objs[i].dx = 0.08;
            g_objs[i].type = 1;
        }
        for (; i < NUMOBJS; ++i) {
            g_objs[i].size = 0.1;
            g_objs[i].dx = 0.05;
            g_objs[i].type = 2;
        }
        for (i = 0; i < NUMOBJS; ++i) {
            do {
                g_objs[i].x = sg_gfrand() * 0.75f + 0.125f;
                g_objs[i].y = sg_gfrand() * 0.75f + 0.125f;
                for (j = 0; j < i; ++j) {
                    r = (g_objs[i].size + g_objs[j].size) * 0.6f;
                    if (fabs(g_objs[i].x - g_objs[j].x) < r &&
                        fabs(g_objs[i].y - g_objs[j].y) < r)
                        break;
                }
            } while (i != j);
            a = sg_gfrand() * (8.0f * atanf(1.0f));
            r = g_objs[i].dx;
            g_objs[i].dx = cosf(a) * r;
            g_objs[i].dy = sinf(a) * r;
        }
    }

    while (g_tick < msec) {
        for (i = 0; i < NUMOBJS; ++i) {
            px = g_objs[i].x + g_objs[i].dx * 0.01;
            py = g_objs[i].y + g_objs[i].dy * 0.01;
            for (j = 0; j < NUMOBJS; ++j) {
                if (i == j)
                    continue;
                r = (g_objs[i].size + g_objs[j].size) * 0.5f;
                dx = fabs(px - g_objs[j].x);
                dy = fabs(py - g_objs[j].y);
                if (dx < r && dy < r) {
                    if (dx < dy)
                        g_objs[i].dy *= -1.0f;
                    else
                        g_objs[i].dx *= -1.0f;
                    goto collision;
                }
            }
            r = g_objs[i].size * 0.5f;
            if (px < r || px > 1 - r) {
                g_objs[i].dx *= -1.0f;
                goto collision;
            }
            if (py < r || py > 1 - r) {
                g_objs[i].dy *= -1.0f;
                goto collision;
            }
            g_objs[i].x = px;
            g_objs[i].y = py;
            continue;

        collision:
            sidx = sg_girand() % 3;
            switch (g_objs[i].type) {
            default:
            case 0: snd = g_snd_tink[sidx]; break;
            case 1: snd = g_snd_clank[sidx]; break;
            case 2: snd = g_snd_donk[sidx]; break;
            }
            sg_audio_source_play(g_snd_source, g_tick, snd, 0);
        }
        g_tick += 10;
    }

    for (i = 0; i < NUMOBJS; ++i) {
        glPushMatrix();
        glTranslatef(g_objs[i].x, g_objs[i].y, 0.0f);
        r = g_objs[i].size * 0.5f;
        switch (g_objs[i].type) {
        default:
        case 0: glColor3ub(200, 220, 255); break;
        case 1: glColor3ub(200, 180,  40); break;
        case 2: glColor3ub(100,  20, 255); break;
        }
        glBegin(GL_TRIANGLE_STRIP);
        glVertex2f( r,  r);
        glVertex2f( r, -r);
        glVertex2f(-r,  r);
        glVertex2f(-r, -r);
        glEnd();
        glPopMatrix();
    }
}

const struct st_iface ST_AUDIO = {
    "Audio 1",
    st_audio_init,
    st_audio_destroy,
    st_audio_event,
    st_audio_draw
};
