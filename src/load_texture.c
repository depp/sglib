/* Copyright 2013 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "defs.h"
#include "sg/file.h"
#include "sg/pixbuf.h"
#include "sg/log.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

struct texture_fmt {
    GLenum ifmt, fmt, type;
    GLint swizzle[4];
};

static const struct texture_fmt TEXTURE_FMT[SG_PIXBUF_NFORMAT] = {
    /* SG_Y: Grayscale */
    { GL_R8, GL_RED, GL_UNSIGNED_BYTE,
      { GL_RED, GL_RED, GL_RED, GL_ONE } },

    /* SG_YA: Grayscale with alpha */
    { GL_RG8, GL_RG, GL_UNSIGNED_BYTE,
      { GL_RED, GL_RED, GL_RED, GL_GREEN } },

    /* SG_RGB: RGB */
    { GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE,
      { GL_RED, GL_GREEN, GL_BLUE, GL_ONE } },

    /* SG_RGBX: RGB with alpha skipped */
    { GL_RGB8, GL_RGBA, GL_UNSIGNED_BYTE,
      { GL_RED, GL_GREEN, GL_BLUE, GL_ONE } },

    /* SG_RGBA: RGB with alpha */
    { GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE,
      { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } }
};

GLuint
load_pixbuf(struct sg_pixbuf *pixbuf, int do_swizzle)
{
    const struct texture_fmt *fmt;
    GLuint texture;

    fmt = &TEXTURE_FMT[pixbuf->format];

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0,
                 fmt->ifmt, pixbuf->pwidth, pixbuf->pheight, 0,
                 fmt->fmt, fmt->type, pixbuf->data);
    if (do_swizzle)
        glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA,
                         fmt->swizzle);
    glBindTexture(GL_TEXTURE_2D, 0);

    return texture;
}

GLuint
load_texture(const char *path)
{
    struct sg_buffer *buf;
    struct sg_pixbuf pixbuf;
    int r;
    GLuint texture;

    buf = sg_file_get(path, strlen(path), SG_RDONLY,
                      SG_PIXBUF_IMAGE_EXTENSIONS, 64 * 1024 * 1024, NULL);
    if (!buf)
        abort();

    sg_pixbuf_init(&pixbuf);
    r = sg_pixbuf_loadimage(&pixbuf, buf->data, buf->length, NULL);
    if (r)
        abort();
    sg_buffer_decref(buf);
    texture = load_pixbuf(&pixbuf, 1);
    sg_pixbuf_destroy(&pixbuf);

    return texture;
}
