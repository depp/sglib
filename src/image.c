#include "defs.h"
#include "sg/opengl.h"
#include "sg/dispatch.h"
#include "sg/entry.h"
#include "sg/program.h"
#include "sg/shader.h"
#include "sg/texture.h"
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

static struct sg_texture *g_images[NUM_IMG];
static struct sg_texture *g_tex[NUM_TEX];

struct {
    struct sg_shader *vert;
    struct sg_shader *frag;

    GLuint prog;

    GLint a_loc, a_texcoord;
    GLint u_vertoff, u_vertscale, u_texscale, u_texture;

    GLuint array;
} g_prog_tex;

static int g_isloaded;

static void
st_image_loaded(void *cxt)
{
    GLuint prog;
    int r;
    (void) cxt;

    g_isloaded = 1;

    g_prog_tex.prog = prog = glCreateProgram();
    glAttachShader(prog, g_prog_tex.vert->shader);
    glAttachShader(prog, g_prog_tex.frag->shader);
    r = sg_program_link(prog, "texture");
    assert(r == 0);
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
    GLuint buf[2];
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
    struct sg_error *err = NULL;
    char buf[32];
    for (i = 0; i < NUM_IMG; ++i) {
        strcpy(buf, "imgtest/");
        strcat(buf, IMAGE_NAMES[i]);
        g_images[i] = sg_texture_file(buf, strlen(buf), &err);
        assert(g_images[i]);
    }
    for (i = 0; i < NUM_TEX; ++i) {
        strcpy(buf, "tex/");
        strcat(buf, TEX_NAMES[i]);
        g_tex[i] = sg_texture_file(buf, strlen(buf), &err);
        assert(g_tex[i]);
    }

    g_prog_tex.vert = sg_shader_file(
        "shader/textured.vert",
        strlen("shader/textured.vert"),
        GL_VERTEX_SHADER, NULL);
    g_prog_tex.frag = sg_shader_file(
        "shader/textured.frag",
        strlen("shader/textured.frag"),
        GL_FRAGMENT_SHADER, NULL);

    sg_dispatch_sync_queue(
        SG_RESOURCES_LOADED, 0, NULL,
        NULL, st_image_loaded);
}

static void
st_image_destroy(void)
{ }

static void
st_image_event(union sg_event *evt)
{
    (void) evt;
}

#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"

static void
st_image_draw(int x, int y, int width, int height, unsigned msec)
{
    int ix, iy, t;
    float x0, x1, y0, y1, u0, u1, v0, v1, s;
    float xoff, yoff, mod, mod2;

    glViewport(x, y, width, height);

    if (!g_isloaded) {
        glClearColor(0.2f, 0.2f, 0.2f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        return;
    }

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, (double) width, 0, (double) height, 1.0, -1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();


    glPushAttrib(GL_ENABLE_BIT);
    glEnable(GL_TEXTURE_2D);

    t = (msec >> 12) % 3;
    mod = (float) (msec & ((1u << 12) - 1)) / (1 << 12);
    mod2 = (float) (msec & ((1u << 13) - 1)) / (1 << 13);
    glBindTexture(GL_TEXTURE_2D, g_tex[t]->texnum);
    s = 1.0f / 256.0f;
    u1 = floorf((float)  width / 2); u0 = u1 - (float)  width;
    v1 = floorf((float) height / 2); v0 = v1 - (float) height;
    u0 *= s; u1 *= s; v0 *= s; v1 *= s;
    x0 = 0; x1 = (float) width;
    y0 = 0; y1 = (float) height;
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    s = sinf((8 * atanf(1.0f)) * mod);
    s = expf(logf(4) * s);
    glScalef(s, s, s);
    glRotatef(mod2 * 360, 0.0f, 0.0f, 1.0f);
    glBegin(GL_TRIANGLE_STRIP);
    glTexCoord2f(u0, v0); glVertex2f(x0, y0);
    glTexCoord2f(u1, v0); glVertex2f(x1, y0);
    glTexCoord2f(u0, v1); glVertex2f(x0, y1);
    glTexCoord2f(u1, v1); glVertex2f(x1, y1);
    glEnd();
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glPopAttrib();


    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(g_prog_tex.prog);
    glUniform1i(g_prog_tex.u_texture, 0);
    float vertscale[2] = { 32 * 2.0f / width, 32 * 2.0f / height };
    glUniform2fv(g_prog_tex.u_vertscale, 1, vertscale);
    float texscale[2] = { 1.0f, 1.0f };
    glUniform2fv(g_prog_tex.u_texscale, 1, texscale);

    for (ix = 0; ix < 6; ++ix) {
        for (iy = 0; iy < 5; ++iy) {
            t = IMAGE_LOCS[iy][ix];
            if (t < 0)
                continue;

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, g_images[t]->texnum);
            float vertoff[2] = { ix * 4 - 10, (4 - iy) * 4 - 8 };
            glUniform2fv(g_prog_tex.u_vertoff, 1, vertoff);
            glBindVertexArray(g_prog_tex.array);
            glDrawArrays(GL_QUADS, 0, 4);
        }
    }
    glUseProgram(0);
    glDisable(GL_BLEND);

    (void) msec;
}

const struct st_iface ST_IMAGE = {
    "Image",
    st_image_init,
    st_image_destroy,
    st_image_event,
    st_image_draw
};
