/* Copyright 2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "sg/opengl.h"
#include "sg/pixbuf.h"

struct sg_texture_fmt {
    GLenum ifmt, fmt, type;
};

static const struct sg_texture_fmt SG_TEXTURE_FMT[SG_PIXBUF_NFORMAT] = {
    { GL_R8, GL_RED, GL_UNSIGNED_BYTE },
    { GL_RG8, GL_RG, GL_UNSIGNED_BYTE },
    { GL_RGB8, GL_RGBA, GL_UNSIGNED_BYTE },
    { GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE }
};

void
sg_pixbuf_texture(struct sg_pixbuf *pbuf)
{
    const struct sg_texture_fmt *fmt;
    fmt = &SG_TEXTURE_FMT[pbuf->format];
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0,
                 fmt->ifmt, pbuf->width, pbuf->height, 0,
                 fmt->fmt, fmt->type, pbuf->data);
}
