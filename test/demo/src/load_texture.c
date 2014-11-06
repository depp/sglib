/* Copyright 2013 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "defs.h"
#include "sg/file.h"
#include "sg/pixbuf.h"
#include "sg/log.h"
#include "sg/util.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

struct texture_fmt {
    GLenum ifmt, fmt, type;
};

static const struct texture_fmt TEXTURE_FMT[SG_PIXBUF_NFORMAT] = {
    { GL_R8, GL_RED, GL_UNSIGNED_BYTE },
    { GL_RG8, GL_RG, GL_UNSIGNED_BYTE },
    { GL_RGB8, GL_RGBA, GL_UNSIGNED_BYTE },
    { GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE }
};

GLuint
load_pixbuf(struct sg_pixbuf *pixbuf)
{
    const struct texture_fmt *fmt;
    GLuint texture;

    fmt = &TEXTURE_FMT[pixbuf->format];

    sg_opengl_checkerror("load_pixbuf start");

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0,
                 fmt->ifmt, pixbuf->width, pixbuf->height, 0,
                 fmt->fmt, fmt->type, pixbuf->data);
    glBindTexture(GL_TEXTURE_2D, 0);

    sg_opengl_checkerror("load_pixbuf");

    return texture;
}

GLuint
load_texture(const char *path)
{
    struct sg_image *image;
    struct sg_pixbuf pixbuf;
    int r;
    unsigned twidth, theight;
    GLuint texture;

    image = sg_image_file(path, strlen(path), NULL);
    if (!image)
        abort();
    twidth = sg_round_up_pow2_32(image->width);
    theight = sg_round_up_pow2_32(image->height);
    if (!twidth || twidth > INT_MAX || !theight || theight > INT_MAX)
        abort();
    r = sg_pixbuf_calloc(&pixbuf, SG_RGBA, (int) twidth, (int) theight, NULL);
    if (r)
        abort();
    r = image->draw(image, &pixbuf, 0, 0, NULL);
    if (r)
        abort();
    image->free(image);
    texture = load_pixbuf(&pixbuf);
    free(pixbuf.data);
    return texture;
}
