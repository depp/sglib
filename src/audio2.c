#include "base/audio_file.h"
#include "base/audio_source.h"
#include "base/entry.h"
#include "base/event.h"
#include "base/keycode/keycode.h"
#include "base/opengl.h"
#include <assert.h>
// #include <math.h>
// #include <stddef.h>
// #include <stdio.h>
#include <string.h>

static struct sg_audio_file *g_snd_clank;
static struct sg_audio_file *g_snd_donk;
static struct sg_audio_file *g_snd_tink;

static unsigned g_cmd;

static struct sg_audio_file *
xloadaudio(const char *name)
{
    struct sg_audio_file *fp;
    char buf[16];
    strcpy(buf, "fx/");
    strcat(buf, name);
    fp = sg_audio_file_new(buf, NULL);
    assert(fp);
    return fp;
}

void
sg_game_init(void)
{
    g_snd_clank = xloadaudio("clank1");
    g_snd_donk = xloadaudio("donk1");
    g_snd_tink = xloadaudio("tink1");
}

void
sg_game_getinfo(struct sg_game_info *info)
{
    (void) info;
}

void
sg_game_event(union sg_event *evt)
{
    int cmd;
    switch (evt->type) {
    case SG_EVENT_KDOWN:
    case SG_EVENT_KUP:
        cmd = -1;
        switch (evt->key.key) {
        case KEY_1: cmd = 0; break;
        case KEY_2: cmd = 1; break;
        case KEY_3: cmd = 2; break;
        case KEY_4: cmd = 3; break;
        case KEY_5: cmd = 4; break;
        case KEY_6: cmd = 5; break;
        case KEY_7: cmd = 6; break;
        case KEY_8: cmd = 7; break;
        case KEY_9: cmd = 8; break;
        case KEY_0: cmd = 9; break;
        default: break;
        }
        if (cmd >= 0) {
            if (evt->type == SG_EVENT_KDOWN)
                g_cmd |= 1u << cmd;
            else
                g_cmd &= ~(1u << cmd);
        }
        break;

    default:
        break;
    }
}

static void
draw_key(int key, int state)
{
    float sz = 0.9f * 0.1f;
    (void) state;

    glPushAttrib(GL_CURRENT_BIT);
    glPushMatrix();
    glTranslatef(-0.9f + 0.2f * key, -0.9f, 0.0f);
    glScalef(sz, sz, sz);

    if (state)
        glColor3ub(240, 90, 30);
    else
        glColor3ub(10, 5, 0);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(-1.0, -1.0);
    glVertex2f(+1.0, -1.0);
    glVertex2f(+1.0, +1.0);
    glVertex2f(-1.0, +1.0);
    glEnd();

    if (state)
        glColor3ub(255, 0, 0);
    else
        glColor3ub(40, 0, 0);
    glBegin(GL_LINE_LOOP);
    glVertex2f(-1.0, -1.0);
    glVertex2f(+1.0, -1.0);
    glVertex2f(+1.0, +1.0);
    glVertex2f(-1.0, +1.0);
    glEnd();

    glPopMatrix();
    glPopAttrib();
}

static int
get_state(int n)
{
    return (g_cmd & (1u << n)) != 0;
}

static void
func1(unsigned msec)
{
    static int g_chan = -1, g_state, g_mod;
    int state = get_state(0);
    struct sg_audio_file *fp;

    if (state && !g_state) {
        if (g_chan < 0)
            g_chan = sg_audio_source_open();
        fp = NULL;
        switch (g_mod) {
        case 0: fp = g_snd_clank; break;
        case 1: fp = g_snd_donk;  break;
        case 2: fp = g_snd_tink;  break;
        }
        g_mod = (g_mod + 1) % 3;
        sg_audio_source_play(g_chan, msec, fp, 0);
    }

    draw_key(0, state);
    g_state = state;
}

static void
func2(unsigned msec)
{
    static int g_chan = -1, g_state, g_mod;
    int state = get_state(1);
    struct sg_audio_file *fp;
    float pan = 0.0f;

    if (state && !g_state) {
        if (g_chan < 0)
            g_chan = sg_audio_source_open();
        fp = NULL;
        switch (g_mod / 3) {
        case 0: fp = g_snd_clank; break;
        case 1: fp = g_snd_donk;  break;
        case 2: fp = g_snd_tink;  break;
        }
        switch (g_mod % 3) {
        case 0: pan = 0.0f; break;
        case 1: pan = -1.0f; break;
        case 2: pan = 1.0f; break;
        }
        g_mod = (g_mod + 1) % 9;
        sg_audio_source_pset(g_chan, SG_AUDIO_PAN, msec, pan);
        sg_audio_source_play(g_chan, msec, fp, 0);
    }

    draw_key(1, state);
    g_state = state;
}

void
sg_game_draw(int x, int y, int width, int height, unsigned msec)
{
    glClearColor(0.08, 0.05, 0.16, 0.0);
    glViewport(x, y, width, height);
    glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    func1(msec);
    func2(msec);

    sg_audio_source_commit(msec, msec);
}

void
sg_game_destroy(void)
{ }
