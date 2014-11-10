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

#define MAX_WIDTH 16384

struct sg_textlayout_line {
    unsigned start;
    unsigned end;
    int width;
    short ascender, descender;
};

static int
sg_textlayout_layout(
    struct sg_textflow *flow,
    struct sg_textpoint *gloc,
    struct sg_textrect *bounds,
    int *baseline)
{
    struct sg_textflow_run *run = flow->run;
    struct sg_textflow_glyph *glyph = flow->glyph;
    struct sg_font *font;
    struct sg_font_glyph *fglyph;
    struct sg_textlayout_line *line = NULL, *nline;
    unsigned ridx, rcount = flow->runcount,
        gidx, gend, gcount = flow->glyphcount, gbreak, gline,
        lidx, lcount, lalloc, nalloc;
    short *adv = NULL;
    short maxx, miny, ascender, descender;
    int pos, bpos, width;

    /* Calculate glyph advances.  */
    adv = malloc(sizeof(*adv) * gcount);
    if (!adv)
        goto nomem;
    gidx = 0;
    for (ridx = 0; ridx < rcount; ridx++) {
        font = run[ridx].font;
        fglyph = font->glyph;
        gend = gidx + run[ridx].count;
        for (; gidx != gend; gidx++)
            adv[gidx] = fglyph[glyph[gidx].index].advance;
    }

    /* Calculate glyph horizontal positions and line breaks.  */
    width = flow->width;
    if (width < 1 || width > MAX_WIDTH)
        width = MAX_WIDTH;
    pos = 0;
    gline = 0;
    gidx = 0;
    lcount = 0;
    lalloc = 0;
    while (1) {
        if (gidx >= gcount)
            goto endline;
        gloc[gidx].x = pos;
        pos += adv[gidx];
        gidx++;
        if (pos <= width)
            continue;
        /* bpos = width from gline..gbreak (half-open range) */
        gbreak = gidx;
        bpos = pos;
        /* Scan backwards for most recent break */
        do {
            gbreak--;
            bpos -= adv[gbreak];
        } while (gbreak > gline &&
               (glyph[gbreak].flags & SG_TEXTFLOW_SPACE) == 0);
        if (gbreak > gline) {
            /* Scan backwards through run of spaces */
            gidx = gbreak;
            pos = bpos;
        endline:
            while (gidx > gline &&
                   (glyph[gidx - 1].flags & SG_TEXTFLOW_SPACE) != 0) {
                gidx--;
                pos -= adv[gidx];
            }
        } else {
            /* No break found by scanning backwards, scan forwards */
            while (gidx < gcount &&
                   (glyph[gidx].flags & SG_TEXTFLOW_SPACE) == 0) {
                gloc[gidx].x = pos;
                pos += adv[gidx];
                gidx++;
            }
        }
        bpos = pos;
        /* Consume trailing spaces */
        while (gidx < gcount &&
               (glyph[gidx].flags & SG_TEXTFLOW_SPACE) != 0) {
            gloc[gidx].x = pos;
            pos += adv[gidx];
            gidx++;
        }
        /* Write line data */
        if (lcount >= lalloc) {
            nalloc = lalloc ? lalloc * 2 : 4;
            if (!nalloc)
                goto nomem;
            nline = realloc(line, sizeof(*line) * nalloc);
            if (!nline)
                goto nomem;
            line = nline;
            lalloc = nalloc;
        }
        line[lcount].start = gline;
        line[lcount].end = gidx;
        line[lcount].width = bpos;
        line[lcount].ascender = 0;
        line[lcount].descender = 0;
        lcount++;
        if (gidx >= gcount)
            break;
        gline = gidx;
        pos = 0;
    }

    /* Calculate vertical positions and bounding box */
    gidx = 0;
    lidx = 0;
    for (ridx = 0; ridx < rcount; ridx++) {
        gend = gidx + run[ridx].count;
        font = run[ridx].font;
        ascender = font->ascender;
        descender = font->descender;
        while (1) {
            if (ascender > line[lidx].ascender)
                line[lidx].ascender = ascender;
            if (descender < line[lidx].descender)
                line[lidx].descender = descender;
            if (gend < line[lidx].end)
                break;
            if (gend == line[lidx].end) {
                lidx++;
                break;
            }
            lidx++;
        }
    }
    miny = maxx = 0;
    pos = 0;
    for (lidx = 0; lidx < lcount; lidx++) {
        pos -= line[lidx].ascender;
        gidx = line[lidx].start;
        gend = line[lidx].end;
        for (; gidx < gend; gidx++)
            gloc[gidx].y = pos;
        pos += line[lidx].descender;
        if (pos < miny) miny = pos;
        if (line[lidx].width > maxx) maxx = line[lidx].width;
    }
    bounds->x0 = 0;
    bounds->y0 = miny;
    bounds->x1 = maxx;
    bounds->y1 = 0;
    *baseline = -line[0].ascender;
    free(adv);
    free(line);
    return 0;

nomem:
    free(adv);
    free(line);
    return -1;
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
    if (sg_textlayout_layout(flow, gloc, &bounds, &baseline))
        goto nomem;

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
