/* Copyright 2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "private.h"
#include "sg/error.h"
#include "sg/type.h"
#include <math.h>
#include <stdlib.h>

struct sg_textflow *
sg_textflow_new(struct sg_error **err)
{
    struct sg_textflow *flow;
    flow = malloc(sizeof(*flow));
    if (!flow) {
        sg_error_nomem(err);
        return NULL;
    }
    flow->err = NULL;
    flow->run = NULL;
    flow->runcount = 0;
    flow->runalloc = 0;
    flow->glyph = NULL;
    flow->glyphcount = 0;
    flow->glyphalloc = 0;
    flow->drawcount = 0;
    flow->width = 0;
    return flow;
}

void
sg_textflow_free(struct sg_textflow *flow)
{
    struct sg_textflow_run *rpos, *rend;
    rpos = flow->run;
    rend = rpos + flow->runcount;
    for (; rpos != rend; rpos++)
        sg_font_decref(rpos->font);
    sg_error_clear(&flow->err);
    free(flow->run);
    free(flow->glyph);
    free(flow);
}

void
sg_textflow_addtext(struct sg_textflow *flow,
                    const char *text, size_t length)
{
    struct sg_textflow_glyph *glyph;
    struct sg_textflow_run *run;
    struct sg_font *font;
    struct sg_font_glyph *fglyph;
    const unsigned char *ptr, *end;
    unsigned c, glyphindex, glyphcount, glyphalloc, drawcount, nalloc, flags;
    FT_Face face;

    if (flow->err)
        return;
    if (!flow->runcount) {
        sg_error_sets(&flow->err, &SG_ERROR_GENERIC, 0,
                      "cannot add text without font");
        return;
    }

    run = &flow->run[flow->runcount-1];
    font = run->font;
    fglyph = font->glyph;
    face = font->typeface->face;
    glyph = flow->glyph;
    glyphcount = flow->glyphcount;
    glyphalloc = flow->glyphalloc;
    drawcount = 0;
    ptr = (const unsigned char *) text;
    end = ptr + length;
    while (ptr < end) {
        c = *ptr;
        if (c < 0x80) {
            ptr += 1;
        } else if ((c & 0xe0) == 0xc0) {
            if (end - ptr < 2)
                break;
            c = ((c & 0x1f) << 6) | (ptr[1] & 0x3f);
            ptr += 2;
        } else if ((c & 0xf0) == 0xe0) {
            if (end - ptr < 3)
                break;
            c = ((c & 0x0f) << 12) |
                ((ptr[1] & 0x3f) << 6) |
                (ptr[2] & 0x3f);
            ptr += 3;
        } else if ((c & 0xf8) == 0xf0) {
            if (end - ptr < 4)
                break;
            c = ((c & 0x07) << 18) |
                ((ptr[1] & 0x3f) << 12) |
                ((ptr[2] & 0x3f) << 6) |
                (ptr[3] & 0x3f);
            ptr += 4;
        } else {
            break;
        }

        glyphindex = FT_Get_Char_Index(face, c);
        if (glyphcount >= glyphalloc) {
            nalloc = glyphalloc ? glyphalloc * 2 : 8;
            if (!nalloc)
                goto nomem;
            glyph = realloc(glyph, sizeof(*glyph) * nalloc);
            if (!glyph)
                goto nomem;
            flow->glyph = glyph;
            flow->glyphalloc = glyphalloc = nalloc;
        }
        if (fglyph[glyphindex].w != 0) {
            drawcount++;
            flags = SG_TEXTFLOW_VISIBLE;
        } else {
            flags = 0;
        }
        if (c == ' ')
            flags |= SG_TEXTFLOW_SPACE;
        glyph[glyphcount].index = glyphindex;
        glyph[glyphcount].flags = flags;
        glyphcount++;
    }

    run->count += glyphcount - flow->glyphcount;
    run->drawcount += drawcount;
    flow->glyphcount = glyphcount;
    flow->drawcount += drawcount;

    return;

nomem:
    sg_error_nomem(&flow->err);
    return;
}

void
sg_textflow_setfont(struct sg_textflow *flow, struct sg_font *font)
{
    unsigned nalloc;
    struct sg_textflow_run *nrun;
    if (flow->err)
        return;
    if (flow->runcount > 0) {
        nrun = &flow->run[flow->runcount - 1];
        if (nrun->font == font)
            return;
        if (nrun->count == 0) {
            sg_font_decref(nrun->font);
            flow->runcount--;
        }
    }
    if (flow->runcount >= flow->runalloc) {
        nalloc = flow->runalloc ? flow->runalloc * 2 : 4;
        if (!nalloc)
            goto nomem;
        nrun = realloc(flow->run, sizeof(*flow->run) * nalloc);
        if (!nrun)
            goto nomem;
        flow->run = nrun;
        flow->runalloc = nalloc;
    }
    nrun = &flow->run[flow->runcount];
    nrun->font = font;
    nrun->count = 0;
    nrun->drawcount = 0;
    flow->runcount++;
    sg_font_incref(font);
    return;

nomem:
    sg_error_nomem(&flow->err);
}

void
sg_textflow_setwidth(struct sg_textflow *flow, float width)
{
    flow->width = (int) floorf(width + 0.5f);
}
