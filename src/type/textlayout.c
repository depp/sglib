/* Copyright 2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "private.h"
#include "sg/error.h"
#include "sg/opengl.h"
#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

/* Calculate glyph locations.  Fills in the locs array.  */
static void
sg_textlayout_layout(struct sg_textflow *flow, struct sg_textpoint *gloc,
                     struct sg_textrect *bounds, int *baseline)
{
    struct sg_textflow_glyph *glyph = flow->glyph;
    struct sg_textflow_run *run = flow->run;
    struct sg_font *font;
    struct sg_font_glyph *fglyph;
    unsigned ridx, rcount = flow->runcount, gidx = 0, gend;
    int pos = 0;
    short bx0, bx1, by0, by1, y0, y1;
    bx0 = by0 = SHRT_MAX;
    bx1 = by1 = SHRT_MIN;
    for (ridx = 0; ridx < rcount; ridx++) {
        font = run[ridx].font;
        fglyph = font->glyph;
        gend = gidx + run[ridx].count;
        y0 = font->descender;
        y1 = font->ascender;
        if (y0 < by0) by0 = y0;
        if (y1 > by1) by1 = y1;
        for (; gidx != gend; gidx++) {
            if (pos < bx0) bx0 = pos;
            gloc[gidx].x = pos;
            gloc[gidx].y = 0;
            pos += fglyph[glyph[gidx].index].advance;
            if (pos > bx1) bx1 = pos;
        }
    }
    bounds->x0 = bx0;
    bounds->y0 = by0;
    bounds->x1 = bx1;
    bounds->y1 = by1;
    *baseline = 0;
}

struct sg_textlayout *
sg_textlayout_new(struct sg_textflow *flow, struct sg_error **err)
{
    short *attr, *apos;
    struct sg_font *font;
    struct sg_font_glyph *fglyph, g;
    struct sg_textflow_run *run;
    struct sg_textflow_glyph *glyph;
    struct sg_textlayout_batch *batch;
    struct sg_textpoint *gloc;
    struct sg_textrect *r, bounds;
    unsigned bidx, bcount, ridx, rcount, drawcount, gidx, gend;
    short vx0, vx1, vy0, vy1, tx0, tx1, ty0, ty1;
    short bx0, bx1, by0, by1;
    int baseline;
    struct sg_textlayout *layout;

    if (flow->err) {
        sg_error_move(err, &flow->err);
        return NULL;
    }

    if (flow->drawcount == 0) {
        layout = malloc(sizeof(*layout));
        if (!layout) {
            sg_error_nomem(err);
            return NULL;
        }
        memset(&layout->metrics, 0, sizeof(layout->metrics));
        layout->buffer = 0;
        layout->batchcount = 0;
        layout->batch = NULL;
        return layout;
    }

    gloc = NULL;
    batch = NULL;
    attr = NULL;
    run = flow->run;
    rcount = flow->runcount;
    glyph = flow->glyph;

    /* Calculate glyph positions */
    gloc = malloc(sizeof(*gloc) * flow->glyphcount);
    if (!gloc)
        goto nomem;
    sg_textlayout_layout(flow, gloc, &bounds, &baseline);

    /* Assign runs to batches */
    bcount = 0;
    batch = malloc(sizeof(*batch) * rcount);
    if (!batch)
        goto nomem;
    for (ridx = 0; ridx < rcount; ridx++) {
        if (run[ridx].drawcount == 0)
            continue;
        font = run[ridx].font;
        for (bidx = 0; bidx < bcount; bidx++)
            if (font == batch[bidx].font)
                break;
        if (bidx < bcount) {
            batch[bidx].count += run[ridx].drawcount;
        } else {
            batch[bidx].font = font;
            batch[bidx].count = run[ridx].drawcount;
            bcount++;
        }
    }

    /* Calculate batch offsets */
    drawcount = 0;
    for (bidx = 0; bidx < bcount; bidx++) {
        batch[bidx].offset = drawcount;
        drawcount += batch[bidx].count;
        batch[bidx].count = 0;
    }
    assert(drawcount == flow->drawcount);

    /* Write glyphs to array */
    bx0 = by0 = SHRT_MAX;
    bx1 = by1 = SHRT_MIN;
    attr = malloc(sizeof(*attr) * 24 * drawcount);
    if (!attr) {
        sg_error_nomem(err);
        return NULL;
    }
    gidx = 0;
    for (ridx = 0; ridx < rcount; ridx++) {
        gend = gidx + run[ridx].count;
        if (run[ridx].drawcount == 0) {
            gidx = gend;
            continue;
        }
        font = run[ridx].font;
        for (bidx = 0; font != batch[bidx].font; bidx++) { }
        apos = attr + (batch[bidx].offset + batch[bidx].count) * 24;
        batch[bidx].count += run[ridx].drawcount;
        fglyph = font->glyph;
        for (; gidx != gend; gidx++) {
            if ((glyph[gidx].flags & SG_TEXTFLOW_VISIBLE) == 0)
                continue;
            g = fglyph[glyph[gidx].index];
            vx0 = gloc[gidx].x + g.bx; vx1 = vx0 + g.w;
            vy1 = gloc[gidx].y + g.by; vy0 = vy1 - g.h;
            tx0 = g.x; tx1 = tx0 + g.w;
            ty1 = g.y; ty0 = ty1 + g.h;
            apos[ 0] = vx0; apos[ 1] = vy0; apos[ 2] = tx0; apos[ 3] = ty0;
            apos[ 4] = vx1; apos[ 5] = vy0; apos[ 6] = tx1; apos[ 7] = ty0;
            apos[ 8] = vx0; apos[ 9] = vy1; apos[10] = tx0; apos[11] = ty1;
            apos[12] = vx0; apos[13] = vy1; apos[14] = tx0; apos[15] = ty1;
            apos[16] = vx1; apos[17] = vy0; apos[18] = tx1; apos[19] = ty0;
            apos[20] = vx1; apos[21] = vy1; apos[22] = tx1; apos[23] = ty1;
            apos += 24;
            if (vx0 < bx0) bx0 = vx0;
            if (vy0 < by0) by0 = vy0;
            if (vx1 > bx1) bx1 = vx1;
            if (vy1 > by1) by1 = vy1;
        }
    }

    /* Convert glyph counts to vertex counts */
    for (bidx = 0; bidx < bcount; bidx++) {
        batch[bidx].offset *= 6;
        batch[bidx].count *= 6;
    }

    /* Create layout object */
    layout = malloc(sizeof(*layout) + sizeof(*batch) * bcount);
    if (!layout)
        goto nomem;
    layout->metrics.logical = bounds;
    r = &layout->metrics.pixel;
    r->x0 = bx0; r->y0 = by0; r->x1 = bx1; r->y1 = by1;
    layout->metrics.baseline = baseline;
    glGenBuffers(1, &layout->buffer);
    glBindBuffer(GL_ARRAY_BUFFER, layout->buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(short) * 24 * drawcount, attr,
                 GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    layout->batchcount = bcount;
    layout->batch = (struct sg_textlayout_batch *) (layout + 1);
    memcpy(layout->batch, batch, sizeof(*batch) * bcount);
    for (bidx = 0; bidx < bcount; bidx++)
        sg_font_incref(batch[bidx].font);
    goto done;

done:
    free(attr);
    free(batch);
    free(gloc);
    return layout;

nomem:
    sg_error_nomem(err);
    layout = NULL;
    goto done;
}

void
sg_textlayout_free(struct sg_textlayout *layout)
{
    struct sg_textlayout_batch *bp, *be;
    bp = layout->batch;
    be = bp + layout->batchcount;
    for (; bp != be; bp++)
        sg_font_decref(bp->font);
    glDeleteBuffers(1, &layout->buffer);
    free(layout);
}

void
sg_textlayout_getmetrics(struct sg_textlayout *layout,
                         struct sg_textmetrics *metrics)
{
    memcpy(metrics, &layout->metrics, sizeof(*metrics));
}

void
sg_textlayout_draw(struct sg_textlayout *layout,
                   unsigned attrib, unsigned texture_scale)
{
    struct sg_textlayout_batch *bp, *be;
    struct sg_font *font;
    bp = layout->batch;
    be = bp + layout->batchcount;
    glBindBuffer(GL_ARRAY_BUFFER, layout->buffer);
    glVertexAttribPointer(attrib, 4, GL_SHORT, GL_FALSE, 0, 0);
    for (; bp != be; bp++) {
        font = bp->font;
        glBindTexture(GL_TEXTURE_2D, font->texture);
        glUniform2fv(texture_scale, 1, font->texscale);
        glDrawArrays(GL_TRIANGLES, bp->offset, bp->count);
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}
