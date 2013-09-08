/* Copyright 2013 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "defs.h"
#include "sg/opengl.h"
#include "sg/dispatch.h"
#include "sg/entry.h"
#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

enum {
    PNG_I4,
    PNG_I8,
    PNG_IA4,
    PNG_IA8,
    PNG_RGB8,
    PNG_RGB16,
    PNG_RGBA8,
    PNG_RGBA16,
    PNG_Y1,
    PNG_Y2,
    PNG_Y4,
    PNG_Y8,
    PNG_Y16,
    PNG_YA8,
    PNG_YA16,

    NUM_IMG
};

static const char IMAGE_NAMES[NUM_IMG][12] = {
    "png_i4",
    "png_i8",
    "png_ia4",
    "png_ia8",
    "png_rgb8",
    "png_rgb16",
    "png_rgba8",
    "png_rgba16",
    "png_y1",
    "png_y2",
    "png_y4",
    "png_y8",
    "png_y16",
    "png_ya8",
    "png_ya16"
};

static const signed char IMAGE_LOCS[5][6] = {
    { -1, -1, -1, -1, PNG_Y1, -1 },
    { -1, -1, -1, -1, PNG_Y2, -1 },
    { PNG_I4, PNG_IA4, -1, -1, PNG_Y4, -1 },
    { PNG_I8, PNG_IA8, PNG_RGB8, PNG_RGBA8, PNG_Y8, PNG_YA8 },
    { -1, -1, PNG_RGB16, PNG_RGBA16, PNG_Y16, PNG_YA16 }
};

enum {
    NUM_TEX = 3
};

static const char TEX_NAMES[NUM_TEX][12] = { "brick", "ivy", "roughstone" };

static GLuint g_images[NUM_IMG];
static GLuint g_tex[NUM_TEX];

struct {
    GLuint prog;

    GLint a_loc;
    GLint u_texoff, u_texmat, u_tex1, u_tex2, u_fade;

    GLuint array;
} g_prog_bkg;

static void st_image_load_bkgprog(void)
{
    GLuint prog, buf;

    g_prog_bkg.prog = prog = load_program(
        "shader/bkg.vert", "shader/bkg.frag");
#define ATTR(x) g_prog_bkg.x = glGetAttribLocation(prog, #x)
#define UNIFORM(x) g_prog_bkg.x = glGetUniformLocation(prog, #x)
    ATTR(a_loc);
    UNIFORM(u_texoff);
    UNIFORM(u_texmat);
    UNIFORM(u_tex1);
    UNIFORM(u_tex2);
    UNIFORM(u_fade);
#undef ATTR
#undef UNIFORM
    glUseProgram(0);

    static const short VERTEX[] = {
        -1, -1,
        +1, -1,
        +1, +1,
        -1, +1
    };

    glGenBuffers(1, &buf);
    glBindBuffer(GL_ARRAY_BUFFER, buf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(VERTEX), VERTEX,
                 GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenVertexArrays(1, &g_prog_bkg.array);
    glBindVertexArray(g_prog_bkg.array);

    glBindBuffer(GL_ARRAY_BUFFER, buf);
    glVertexAttribPointer(
        g_prog_bkg.a_loc, 2, GL_SHORT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(g_prog_bkg.a_loc);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(0);
}

struct {
    GLuint prog;

    GLint a_loc, a_texcoord;
    GLint u_vertoff, u_vertscale, u_texscale, u_texture;

    GLuint array;
} g_prog_tex;

static void
st_image_load_fgprog(void)
{
    GLuint prog, buf[2];

    g_prog_tex.prog = prog = load_program(
        "shader/textured.vert", "shader/textured.frag");
#define ATTR(x) g_prog_tex.x = glGetAttribLocation(prog, #x)
#define UNIFORM(x) g_prog_tex.x = glGetUniformLocation(prog, #x)
    ATTR(a_loc);
    ATTR(a_texcoord);
    UNIFORM(u_vertoff);
    UNIFORM(u_vertscale);
    UNIFORM(u_texscale);
    UNIFORM(u_texture);
#undef ATTR
#undef UNIFORM
    glUseProgram(0);

    static const short VERTEX[] = {
        -1, -1,
        +1, -1,
        +1, +1,
        -1, +1
    };
    static const short TEXCOORD[] = {
        0, 1,
        1, 1,
        1, 0,
        0, 0
    };
    glGenBuffers(2, buf);
    glBindBuffer(GL_ARRAY_BUFFER, buf[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(VERTEX), VERTEX,
                 GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, buf[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(TEXCOORD), TEXCOORD,
                 GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenVertexArrays(1, &g_prog_tex.array);
    glBindVertexArray(g_prog_tex.array);

    glBindBuffer(GL_ARRAY_BUFFER, buf[0]);
    glVertexAttribPointer(
        g_prog_tex.a_loc, 2, GL_SHORT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(g_prog_tex.a_loc);
    glBindBuffer(GL_ARRAY_BUFFER, buf[1]);
    glVertexAttribPointer(
        g_prog_tex.a_texcoord, 2, GL_SHORT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(g_prog_tex.a_texcoord);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(0);
}

static void
st_image_init(void)
{
    int i;
    char buf[32];
    for (i = 0; i < NUM_IMG; ++i) {
        strcpy(buf, "imgtest/");
        strcat(buf, IMAGE_NAMES[i]);
        g_images[i] = load_texture(buf);
    }
    for (i = 0; i < NUM_TEX; ++i) {
        strcpy(buf, "tex/");
        strcat(buf, TEX_NAMES[i]);
        g_tex[i] = load_texture(buf);
    }
    st_image_load_bkgprog();
    st_image_load_fgprog();
}

static void
st_image_destroy(void)
{ }

static void
st_image_event(union sg_event *evt)
{
    (void) evt;
}

/* All use column major order */

static void
matrix2_mul(float *x, const float *y, const float *z)
{
    float y11 = y[0], y21 = y[1], y12 = y[2], y22 = y[3];
    float z11 = z[0], z21 = z[1], z12 = z[2], z22 = z[3];
    x[0] = y11 * z11 + y12 * z21;
    x[1] = y21 * z11 + y22 * z21;
    x[2] = y11 * z12 + y12 * z22;
    x[3] = y21 * z12 + y22 * z22;
}

__attribute__((unused))
static void
matrix2_identity(float *x)
{
    x[0] = 1.0f;
    x[1] = 0.0f;
    x[2] = 0.0f;
    x[3] = 1.0f;
}

__attribute__((unused))
static void
matrix2_scale(float *x, float scale1, float scale2)
{
    x[0] = scale1;
    x[1] = 0.0f;
    x[2] = 0.0f;
    x[3] = scale2;
}

__attribute__((unused))
static void
matrix2_rotate(float *x, float angle)
{
    float a = cosf(angle), b = sinf(angle);
    x[0] = a;
    x[1] = b;
    x[2] = -b;
    x[3] = a;
}

static void
st_image_draw_background(int width, int height, unsigned msec)
{
    int t;
    float mod, mod2, s;

    glUseProgram(g_prog_bkg.prog);
    glUniform2f(g_prog_bkg.u_texoff, 0.0f, 0.0f);

    t = (msec >> 12) % 3;
    mod = (float) (msec & ((1u << 12) - 1)) / (1 << 12);
    mod2 = (float) (msec & ((1u << 13) - 1)) / (1 << 13);
    s = -cosf((8 * atanf(1.0f)) * mod);
    s = expf(logf(4) * s);

    float bg_texmat[4], rot[4], mat[4];
    matrix2_rotate(rot, mod2 * (8.0f * atanf(1.0f)));
    matrix2_scale(mat, (float) width * (0.5f / 256) * s,
                  (float) height * (0.5f / 256) * s);
    matrix2_mul(bg_texmat, rot, mat);
    glUniformMatrix2fv(g_prog_bkg.u_texmat, 1, GL_FALSE, bg_texmat);

    glUniform1i(g_prog_bkg.u_tex1, 0);
    glUniform1i(g_prog_bkg.u_tex2, 1);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g_tex[t]);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, g_tex[(t+1) % 3]);
    glUniform1f(g_prog_bkg.u_fade, mod);

    glBindVertexArray(g_prog_bkg.array);
    glDrawArrays(GL_QUADS, 0, 4);

    glUseProgram(0);
}

static void
st_image_draw_foreground(int width, int height)
{
    int ix, iy, t;

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(g_prog_tex.prog);
    glUniform1i(g_prog_tex.u_texture, 0);
    glUniform2f(g_prog_tex.u_vertscale,
                32 * 2.0f / width, 32 * 2.0f / height);
    glUniform2f(g_prog_tex.u_texscale, 1.0f, 1.0f);

    for (ix = 0; ix < 6; ++ix) {
        for (iy = 0; iy < 5; ++iy) {
            t = IMAGE_LOCS[iy][ix];
            if (t < 0)
                continue;

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, g_images[t]);
            glUniform2f(g_prog_tex.u_vertoff,
                        ix * 4 - 10, (4 - iy) * 4 - 8);
            glBindVertexArray(g_prog_tex.array);
            glDrawArrays(GL_QUADS, 0, 4);
        }
    }
    glUseProgram(0);
    glDisable(GL_BLEND);
}

static void
st_image_draw(int x, int y, int width, int height, unsigned msec)
{
    glViewport(x, y, width, height);
    st_image_draw_background(width, height, msec);
    st_image_draw_foreground(width, height);
}

const struct st_iface ST_IMAGE = {
    "Image",
    st_image_init,
    st_image_destroy,
    st_image_event,
    st_image_draw
};
