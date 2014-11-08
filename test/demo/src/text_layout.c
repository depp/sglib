/* Copyright 2013 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "defs.h"
#include "sg/pixbuf.h"
#include "sg/util.h"
#include <limits.h>
#include <stdlib.h>

void
text_layout_create(struct text_layout *layout,
                   const char *text, size_t textlen,
                   const char *fontname, double fontsize,
                   sg_textalign_t alignment, double width)
{
    struct sg_textbitmap *bitmap;
    struct sg_textlayout_metrics metrics;
    struct sg_pixbuf pixbuf;
    int r, x0, x1, y0, y1;
    unsigned tw, th;
    struct sg_textpoint offset;
    GLuint texture, buffer;
    short v[4][4];

    bitmap = sg_textbitmap_new_simple(text, textlen, fontname, fontsize,
                                      alignment, width, NULL);
    if (!bitmap) abort();

    r = sg_textbitmap_getmetrics(bitmap, &metrics, NULL);
    if (r) abort();

    x0 = metrics.pixel.x0;
    x1 = metrics.pixel.x1;
    y0 = metrics.pixel.y0;
    y1 = metrics.pixel.y1;

    tw = sg_round_up_pow2_32(x1 - x0);
    th = sg_round_up_pow2_32(y1 - y0);
    if (!tw || tw > INT_MAX || !th || th > INT_MAX)
        abort();
    r = sg_pixbuf_calloc(&pixbuf, SG_R, (int) tw, (int) th, NULL);
    if (r) abort();
    offset.x = -x0;
    offset.y = (int) th - y1;
    r = sg_textbitmap_render(bitmap, &pixbuf, offset, NULL);
    if (r) abort();

    texture = load_pixbuf(&pixbuf);

    v[0][0] = x0; v[0][1] = y0; v[0][2] = 0;       v[0][3] = y1 - y0;
    v[1][0] = x1; v[1][1] = y0; v[1][2] = x1 - x0; v[1][3] = y1 - y0;
    v[2][0] = x0; v[2][1] = y1; v[2][2] = 0;       v[2][3] = 0;
    v[3][0] = x1; v[3][1] = y1; v[3][2] = x1 - x0; v[3][3] = 0;

    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    layout->texture = texture;
    layout->buffer = buffer;
    layout->metrics = metrics;
    layout->texture_scale[0] = 1.0 / tw;
    layout->texture_scale[1] = 1.0 / th;
}
