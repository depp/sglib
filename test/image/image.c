#include "base/opengl.h"
#include "base/entry.h"
#include "base/texture.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>

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

    NUMTEX
};

static const char const IMAGE_NAMES[NUMTEX][12] = {
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

static struct sg_texture_image *g_images[NUMTEX];

void
sg_game_init(void)
{
    int i;
    struct sg_error *err = NULL;
    for (i = 0; i < NUMTEX; ++i) {
        g_images[i] = sg_texture_image_new(IMAGE_NAMES[i], &err);
        assert(g_images[i]);
    }
}

void
sg_game_getinfo(struct sg_game_info *info)
{
    (void) info;
}

void
sg_game_event(union sg_event *evt)
{
    (void) evt;
}

void
sg_game_draw(int x, int y, int width, int height, unsigned msec)
{
    int ix, iy, t;
    float x0, x1, y0, y1, u0, u1, v0, v1;

    glClear(GL_COLOR_BUFFER_BIT);
    glViewport(x, y, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, (double) width, 0, (double) height, 1.0, -1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glPushAttrib(GL_ENABLE_BIT);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    for (ix = 0; ix < 6; ++ix) {
        for (iy = 0; iy < 5; ++iy) {
            t = IMAGE_LOCS[iy][ix];
            if (t < 0)
                continue;
            glBindTexture(GL_TEXTURE_2D, g_images[t]->texnum);
            u0 = 0.0f; u1 = 1.0f; v0 = 1.0f; v1 = 0.0f;
            x0 = (float)(32 + 128 * ix); x1 = x0 + 64;
            y0 = (float)(32 + 128 * (4 - iy)); y1 = y0 + 64;
            glColor4ub(255, 255, 255, 255);
            glBegin(GL_TRIANGLE_STRIP);
            glTexCoord2f(u0, v0); glVertex2f(x0, y0);
            glTexCoord2f(u1, v0); glVertex2f(x1, y0);
            glTexCoord2f(u0, v1); glVertex2f(x0, y1);
            glTexCoord2f(u1, v1); glVertex2f(x1, y1);
            glEnd();
        }
    }
    glPopAttrib();

    (void) msec;
}

void
sg_game_destroy(void)
{ }
