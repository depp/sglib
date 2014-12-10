/* Copyright 2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "private.h"
#include "sg/error.h"
#include "sg/type.h"
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

int
sg_textlayout_create(struct sg_textlayout *layout, struct sg_textflow *flow,
                     struct sg_error **err)
{
    struct sg_textvert *vert, *v;
    struct sg_font *font;
    struct sg_font_glyph *fglyph, g;
    struct sg_textflow_run *run;
    struct sg_textflow_glyph *glyph;
    struct sg_textbatch *batch;
    struct sg_textpoint *gloc;
    struct sg_textrect *r, bounds;
    unsigned bidx, bcount, ridx, rcount, drawcount, gidx, gend;
    short vx0, vx1, vy0, vy1, tx0, tx1, ty0, ty1;
    short bx0, bx1, by0, by1;
    int baseline;

    if (flow->err) {
        sg_error_move(err, &flow->err);
        return -1;
    }

    if (flow->drawcount == 0) {
        memset(&layout->metrics, 0, sizeof(layout->metrics));
        layout->vert = NULL;
        layout->vertcount = 0;
        layout->batch = NULL;
        layout->batchcount = 0;
        return 0;
    }

    gloc = NULL;
    batch = NULL;
    vert = NULL;
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
    vert = malloc(sizeof(*vert) * 6 * drawcount);
    if (!vert) {
        sg_error_nomem(err);
        return -1;
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
        v = vert + (batch[bidx].offset + batch[bidx].count) * 24;
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
            v[0].vx = vx0; v[0].vy = vy0; v[0].tx = tx0; v[0].ty = ty0;
            v[1].vx = vx1; v[1].vy = vy0; v[1].tx = tx1; v[1].ty = ty0;
            v[2].vx = vx0; v[2].vy = vy1; v[2].tx = tx0; v[2].ty = ty1;
            v[3].vx = vx0; v[3].vy = vy1; v[3].tx = tx0; v[3].ty = ty1;
            v[4].vx = vx1; v[4].vy = vy0; v[4].tx = tx1; v[4].ty = ty0;
            v[5].vx = vx1; v[5].vy = vy1; v[5].tx = tx1; v[5].ty = ty1;
            v += 6;
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
    layout->metrics.logical = bounds;
    r = &layout->metrics.pixel;
    r->x0 = bx0; r->y0 = by0; r->x1 = bx1; r->y1 = by1;
    layout->metrics.baseline = baseline;
    layout->vert = vert;
    layout->vertcount = drawcount * 6;
    layout->batch = batch;
    layout->batchcount = bcount;

    free(gloc);
    return 0;

nomem:
    sg_error_nomem(err);
    free(vert);
    free(batch);
    free(gloc);
    return -1;
}

void
sg_textlayout_destroy(struct sg_textlayout *layout)
{
    free(layout->vert);
    free(layout->batch);
}
