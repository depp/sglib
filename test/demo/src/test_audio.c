/* Copyright 2013-2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "defs.h"
#include "sg/mixer.h"
#include "sg/entry.h"
#include "sg/event.h"
#include "sg/opengl.h"
#include "sg/keycode.h"
#include <stdlib.h>
#include <string.h>

static GLuint g_buf;
static struct prog_plain g_prog_plain;

struct {
    struct sg_mixer_sound *clank;
    struct sg_mixer_sound *donk;
    struct sg_mixer_sound *tink;
    struct sg_mixer_sound *stereo;
    struct sg_mixer_sound *left;
    struct sg_mixer_sound *right;
    struct sg_mixer_sound *music;
    struct sg_mixer_sound *alien;
} g_audio;

static unsigned g_cmd;

static const float DATA[] = {
    -0.9f, -0.9f,
    -0.9f, +0.9f,
    +0.9f, +0.9f,
    +0.9f, -0.9f
};

static void
st_audio_init(void)
{
    g_audio.clank  = load_audio("fx/clank1");
    g_audio.donk   = load_audio("fx/donk1");
    g_audio.tink   = load_audio("fx/tink1");
    g_audio.stereo = load_audio("fx/stereo");
    g_audio.left   = load_audio("fx/left");
    g_audio.right  = load_audio("fx/right");
    g_audio.music  = load_audio("fx/looptrack");
    g_audio.alien  = load_audio("fx/alien");

    glGenBuffers(1, &g_buf);
    glBindBuffer(GL_ARRAY_BUFFER, g_buf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(DATA), DATA, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    sg_opengl_checkerror("st_audio_init");

    load_prog_plain(&g_prog_plain);
}

static void
st_audio_destroy(void)
{ }

static void
st_audio_event(union sg_event *evt)
{
    int cmd;
    switch (evt->common.type) {
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
            if (evt->key.type == SG_EVENT_KDOWN)
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
    int n = 10;
    double scale = 1.0 / n;

    glUseProgram(g_prog_plain.prog);
    glUniform2f(
        g_prog_plain.u_vertoff, 2*key+1-n, 1-n);
    glUniform2f(g_prog_plain.u_vertscale, scale, scale);
    glBindBuffer(GL_ARRAY_BUFFER, g_buf);
    glEnableVertexAttribArray(g_prog_plain.a_loc);
    glVertexAttribPointer(g_prog_plain.a_loc, 2, GL_FLOAT, GL_FALSE, 0, 0);

    if (state)
        glUniform4f(g_prog_plain.u_color,
                    240 / 255.0, 90 / 255.0, 30 / 255.0, 1.0f);
    else
        glUniform4f(g_prog_plain.u_color,
                    10 / 255.0, 5 / 255.0, 0 / 255.0, 1.0f);

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    if (state)
        glUniform4f(g_prog_plain.u_color,
                    255 / 255.0, 0 / 255.0, 0 / 255.0, 1.0f);
    else
        glUniform4f(g_prog_plain.u_color,
                    40 / 255.0, 0 / 255.0, 0 / 255.0, 1.0f);

    glDrawArrays(GL_LINE_LOOP, 0, 4);

    glDisableVertexAttribArray(g_prog_plain.a_loc);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glUseProgram(0);
}

static int
get_state(int n)
{
    return (g_cmd & (1u << n)) != 0;
}

static void
func1(double time)
{
    static int g_state, g_mod;
    int state = get_state(0);
    struct sg_mixer_sound *snd;

    if (state && !g_state) {
        snd = NULL;
        switch (g_mod) {
        case 0: snd = g_audio.clank; break;
        case 1: snd = g_audio.donk;  break;
        case 2: snd = g_audio.tink;  break;
        }
        g_mod = (g_mod + 1) % 3;
        sg_mixer_channel_play(snd, time, SG_MIXER_FLAG_DETACHED);
    }

    draw_key(0, state);
    g_state = state;
}

static void
func2(double time)
{
    static int g_state, g_mod;
    int state = get_state(1);
    struct sg_mixer_sound *snd;
    struct sg_mixer_channel *chan;
    float pan = 0.0f;

    if (state && !g_state) {
        snd = NULL;
        switch (g_mod / 3) {
        case 0: snd = g_audio.clank; break;
        case 1: snd = g_audio.donk;  break;
        case 2: snd = g_audio.tink;  break;
        }
        switch (g_mod % 3) {
        case 0: pan = 0.0f; break;
        case 1: pan = -1.0f; break;
        case 2: pan = 1.0f; break;
        }
        g_mod = (g_mod + 1) % 9;
        chan = sg_mixer_channel_play(snd, time, SG_MIXER_FLAG_DETACHED);
        sg_mixer_channel_setparam(chan, SG_MIXER_PARAM_PAN, pan);
    }

    draw_key(1, state);
    g_state = state;
}

static void
func3(double time)
{
    static int g_state, g_mod;
    int state = get_state(2);
    struct sg_mixer_sound *snd;
    struct sg_mixer_channel *chan;
    float pan = 0.0f;

    if (state && !g_state) {
        snd = g_audio.stereo;
        snd = NULL;
        switch (g_mod) {
        case 0: snd = g_audio.stereo; pan =  0.00f; break;
        case 1: snd = g_audio.left;   pan = -0.75f; break;
        case 2: snd = g_audio.right;  pan = +0.75f; break;
        }
        g_mod = (g_mod + 1) % 3;
        chan = sg_mixer_channel_play(snd, time, SG_MIXER_FLAG_DETACHED);
        sg_mixer_channel_setparam(chan, SG_MIXER_PARAM_PAN, pan);
    }

    draw_key(2, state);
    g_state = state;
}

static void
func4(double time)
{
    static int g_state;
    int state = get_state(3);
    struct sg_mixer_sound *snd;
    static struct sg_mixer_channel *chan;

    if (chan && sg_mixer_channel_isdone(chan))
        sg_mixer_channel_stop(chan);

    switch (g_state & 3) {
    case 0:
        if (state) {
            snd = (g_state & 4) ? g_audio.alien : g_audio.music;
            chan = sg_mixer_channel_play(snd, time, SG_MIXER_FLAG_LOOP);
            g_state++;
        }
        break;

    case 2:
        if (state) {
            sg_mixer_channel_stop(chan);
            chan = NULL;
            g_state++;
        }
        break;

    default:
        if (!state)
            g_state++;
        break;
    }
    g_state &= 7;

    draw_key(3, state);
}

static void
st_audio_draw(int width, int height, double time)
{
    glClearColor(0.08, 0.05, 0.16, 0.0);
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT);

    func1(time);
    func2(time);
    func3(time);
    func4(time);
}

const struct st_iface ST_AUDIO = {
    "Audio",
    st_audio_init,
    st_audio_destroy,
    st_audio_event,
    st_audio_draw
};
