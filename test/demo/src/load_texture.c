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

GLuint
load_pixbuf(struct sg_pixbuf *pixbuf)
{
    GLuint texture;

    sg_opengl_checkerror("load_pixbuf start");

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    sg_pixbuf_texture(pixbuf);
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
