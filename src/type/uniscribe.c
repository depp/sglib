/* Copyright 2012-2013 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "sg/defs.h"

#include "../core/common/private.h"
#include "sg/error.h"
#include "sg/pixbuf.h"
#include "sg/type.h"
#include "sg/util.h"
#include <Windows.h>
#include <usp10.h>
#include <math.h>
#include <limits.h>

#pragma comment(lib, "Usp10.lib")

static int sg_uniscribe_initted;
static SCRIPT_DIGITSUBSTITUTE sg_uniscribe_psds;
static HDC sg_uniscribe_dc;

/* All coordinates are Windows pixel coordinates,
   i.e., the Y-axis is flipped compared to OpenGL.  */

struct sg_textbitmap_line {
    /* Location of line */
    int posx, posy;

    /* Metrics */
    int width;
    int ascent, descent;

    /* Indexes for runs in this line */
    int runoffset, runcount;
};

struct sg_textbitmap_run {
    SCRIPT_ANALYSIS analysis;

    /* Location of run */
    int posx;

    /* INdexes for glyphs in this run */
    int glyphoffset, glyphcount;
};

struct sg_textbitmap_state {
    /* Maximum width of the layout */
    int width;

    /* Text, in UTF-16 (temporary) */
    int textlen;
    wchar_t *text;

    /* X position of the cursor */
    int posx;

    /* Script items */
    int itemcount, itemalloc;
    SCRIPT_ITEM *item;

    /* Temporary buffer for visattr in the current item */
    int visattralloc;
    SCRIPT_VISATTR *visattr;

    /* Temporary buffer for each code unit in the current item */
    WORD *cluster;

    /* Temporary buffer for attributes for each code unit in the
       current item */
    int logattralloc;
    SCRIPT_LOGATTR *logattr;
};

struct sg_textbitmap {
    HDC dc;
    HFONT font;
    SCRIPT_CACHE cache;

    /* Line info */
    int linecount, linealloc;
    struct sg_textbitmap_line *line;

    /* Run info */
    int runcount, runalloc;
    struct sg_textbitmap_run *run;

    /* Glyph info */
    int glyphcount, glyphalloc;
    WORD *glyph;
    GOFFSET *glyphoffset; /* Locations for each glyph */
    int *glyphadvance; /* Advance of each glyph */
};

static HFONT
sg_textbitmap_getfont(const char *fontname, double fontsize)
{
    wchar_t *wfontname;
    int wfontnamelen;
    LOGFONTW f;

    if (sg_wchar_from_utf8(&wfontname, &wfontnamelen,
                           fontname, strlen(fontname) + 1))
        return NULL;

    f.lfHeight = -(long) floor(fontsize + 0.5);
    f.lfWidth = 0;
    f.lfEscapement = 0;
    f.lfOrientation = 0;
    f.lfWeight = 500;
    f.lfItalic = 0;
    f.lfUnderline = 0;
    f.lfStrikeOut = 0;
    f.lfCharSet = ANSI_CHARSET;
    f.lfOutPrecision = OUT_OUTLINE_PRECIS;
    f.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    f.lfQuality = ANTIALIASED_QUALITY;
    f.lfPitchAndFamily = DEFAULT_PITCH;
    if (wfontnamelen > sizeof(f.lfFaceName) / sizeof(*f.lfFaceName)) {
        free(wfontname);
        return NULL;
    }
    memcpy(f.lfFaceName, wfontname, wfontnamelen * sizeof(wchar_t));
    free(wfontname);

    return CreateFontIndirectW(&f);
}

static void
sg_textbitmap_uniscribe_init(void)
{
    HRESULT hr;
    hr = ScriptRecordDigitSubstitution(LOCALE_USER_DEFAULT,
                                       &sg_uniscribe_psds);
    if (FAILED(hr))
        abort();
}

/* Break the text into items.  */
static int
sg_textbitmap_itemize(struct sg_textbitmap_state *state)
{
    int itemalloc, itemcount;
    HRESULT hr;
    SCRIPT_ITEM *item = NULL;

    /* This is a guess, given by the ScriptItemize documentation.  */
    itemalloc = (unsigned)state->textlen / 16 + 4;
    while (1) {
        if (itemalloc > (size_t)-1 / sizeof(*item))
            return -1;
        item = malloc(sizeof(*item) * (itemalloc + 1));
        if (!item)
            return -1;
        hr = ScriptItemize(
            state->text,
            state->textlen,
            itemalloc,
            NULL,
            NULL,
            item,
            &itemcount);
        if (!FAILED(hr))
            break;
        free(item);
        if (hr != E_OUTOFMEMORY || itemalloc >= state->textlen)
            return -1;
        itemalloc *= 2;
        if (!itemalloc || itemalloc > state->textlen)
            itemalloc = state->textlen;
    }
    state->itemalloc = itemalloc;
    state->itemcount = itemcount;
    state->item = item;
    return 0;
}

/* Allocate arrays for glyphs in the text bitmap.  Count is the
   minimum number of unused glyphs to allocate.  So the postcondition
   is bitmap->glyphalloc >= bitmap->glyphcount + count.  */
static int
sg_textbitmap_allocglyphs(struct sg_textbitmap *bitmap, int count)
{
    WORD *glyph;
    GOFFSET *glyphoffset;
    int *glyphadvance;
    unsigned nalloc;

    if (count < 0)
        return -1;
    if (count == 0)
        return 0;

    if (count > INT_MAX - bitmap->glyphcount)
        return -1;
    nalloc = sg_round_up_pow2_32((unsigned)bitmap->glyphcount + count);
    if (nalloc == 0 || nalloc > INT_MAX)
        return 0;

    glyph = realloc(bitmap->glyph, sizeof(*glyph) * nalloc);
    if (!glyph)
        return -1;
    bitmap->glyph = glyph;

    glyphadvance = realloc(bitmap->glyphadvance,
                           sizeof(*glyphadvance) * nalloc);
    if (!glyphadvance)
        return -1;
    bitmap->glyphadvance = glyphadvance;

    glyphoffset = realloc(bitmap->glyphoffset, sizeof(*glyphoffset) * nalloc);
    if (!glyphoffset)
        return -1;
    bitmap->glyphoffset = glyphoffset;

    bitmap->glyphalloc = nalloc;
    return 0;
}

/* Allocate the temporary visattr table.  */
static int
sg_textbitmap_allocvisattr(struct sg_textbitmap_state *state, int count)
{
    SCRIPT_VISATTR *visattr;
    free(state->visattr);
    state->visattr = NULL;
    visattr = malloc(sizeof(*visattr) * count);
    if (!visattr)
        return -1;
    state->visattr = visattr;
    state->visattralloc = count;
    return 0;
}

/* Shape the current item into glyphs.  Returns the number of glyphs.  */
static int
sg_textbitmap_shape(struct sg_textbitmap *bitmap,
                    struct sg_textbitmap_state *state,
                    int textstart, int textcount,
                    SCRIPT_ANALYSIS *analysis)
{
    int maxglyphs, glyphcount;
    unsigned nalloc;
    HRESULT hr;

    while (1) {
        maxglyphs = bitmap->glyphalloc - bitmap->glyphcount;
        if (maxglyphs > state->visattralloc)
            maxglyphs = state->visattralloc;

        hr = ScriptShape(
            bitmap->dc,
            &bitmap->cache,
            state->text + textstart,
            textcount,
            maxglyphs,
            analysis,
            bitmap->glyph + bitmap->glyphcount,
            state->cluster,
            state->visattr,
            &glyphcount);

        if (!FAILED(hr))
            return glyphcount;
        if (hr != E_OUTOFMEMORY)
            return -1;
        if (maxglyphs == bitmap->glyphalloc - bitmap->glyphcount) {
            if (sg_textbitmap_allocglyphs(bitmap, maxglyphs + 1))
                return -1;
        }
        if (maxglyphs == state->visattralloc) {
            nalloc = sg_round_up_pow2_32((unsigned)maxglyphs + 1);
            if (!nalloc || nalloc > INT_MAX)
                return -1;
            if (sg_textbitmap_allocvisattr(state, (int)nalloc))
                return -1;
        }
    }
}

/* Find the position, in code units, at which to wrap the current item */
static int
sg_textbitmap_wrap(struct sg_textbitmap *bitmap,
                   struct sg_textbitmap_state *state,
                    int textstart, int textcount, int glyphcount,
                   const SCRIPT_ANALYSIS *analysis)
{
    int remx, i, j, advance;
    unsigned nalloc;
    WORD cluster;
    HRESULT hr;

    /* Find the first glyph past the bounding box.  */
    remx = state->width - state->posx;
    for (i = 0; i < glyphcount; ++i) {
        advance = bitmap->glyphadvance[bitmap->glyphcount + i];
        if (advance > remx)
            break;
        remx -= advance;
    }

    /* Find the first character corresponding to a cluster containing
       the glyph past the bounding box.  */
    for (j = 0; j < glyphcount; ++j) {
        cluster = state->cluster[j];
        if (cluster >= i) {
            if (cluster > i && j > 0) {
                j--;
                cluster = state->cluster[j];
                while (j > 0 && state->cluster[j - 1] == cluster)
                    j--;
            }
            break;
        }
    }

    /* Perform break analysis to find a suitable point to break.  */
    if (textcount > state->logattralloc) {
        free(state->logattr);
        state->logattr = NULL;
        nalloc = sg_round_up_pow2_32(textcount);
        if (!nalloc || nalloc > INT_MAX ||
            nalloc > (size_t)-1 / sizeof(*state->logattr))
            return -1;
        state->logattr = malloc(sizeof(*state->logattr) * nalloc);
        if (!state->logattr)
            return -1;
    }

    hr = ScriptBreak(
        state->text + textstart,
        textcount,
        analysis,
        state->logattr);

    /* Scan for a break character no later than j.  */
    i = (j >= textcount) ? textcount - 1 : j;
    for (; i >= 0; --i) {
        if (state->logattr[i].fWhiteSpace) {
            j = i + 1;
            break;
        } else if (state->logattr[i].fSoftBreak) {
            j = i;
            break;
        }
    }

    /* i = # of characters in this line */
    /* j = relative offset of first character in next line */

    return i;
}

/* Start a new line.  Return -1 on failure.  */
static int
sg_textbitmap_newline(struct sg_textbitmap *bitmap)
{
    unsigned nalloc;
    struct sg_textbitmap_line *line;
    if (bitmap->linecount >= bitmap->linealloc) {
        nalloc = sg_round_up_pow2_32((unsigned)bitmap->linecount + 1);
        if (!nalloc || nalloc > INT_MAX ||
            nalloc > (size_t)-1 / sizeof(*line))
            return -1;
        line = realloc(bitmap->line, sizeof(*line) * nalloc);
        if (!line)
            return -1;
        bitmap->line = line;
        bitmap->linealloc = nalloc;
    }
    line = &bitmap->line[bitmap->linecount++];
    line->posx = 0;
    line->posy = 0;
    line->width = 0;
    line->ascent = 0;
    line->descent = 0;
    line->runoffset = bitmap->runcount;
    line->runcount = 0;
    return 0;
}

/* Create a new run and return a pointer to it, or NULL on failure.  */
static struct sg_textbitmap_run *
sg_textbitmap_newrun(struct sg_textbitmap *bitmap)
{
    unsigned nalloc;
    struct sg_textbitmap_run *run;
    if (bitmap->runcount >= bitmap->runalloc) {
        nalloc = sg_round_up_pow2_32((unsigned)bitmap->runcount + 1);
        if (!nalloc || nalloc > INT_MAX || nalloc > (size_t)-1 / sizeof(*run))
            return NULL;
        run = realloc(bitmap->run, sizeof(*run) * nalloc);
        if (!run)
            return NULL;
        bitmap->run = run;
        bitmap->runalloc = nalloc;
    }
    return &bitmap->run[bitmap->runcount++];
}

/* Place an item.  Returns the number of characters placed, or -1 on
   failure.  */
static int
sg_textbitmap_place(struct sg_textbitmap *bitmap,
                    struct sg_textbitmap_state *state,
                    int textstart, int textcount,
                    SCRIPT_ANALYSIS itemanalysis,
                    const TEXTMETRIC *metrics)
{
    HRESULT hr;
    ABC abc;
    int width;
    int breakpos;
    int glyphcount;
    struct sg_textbitmap_run *run;
    struct sg_textbitmap_line *line;
    SCRIPT_ANALYSIS analysis;

    analysis = itemanalysis;
    glyphcount = sg_textbitmap_shape(bitmap, state, textstart, textcount,
                                     &analysis);
    if (glyphcount < 0)
        return -1;

    hr = ScriptPlace(
        bitmap->dc,
        &bitmap->cache,
        bitmap->glyph + bitmap->glyphcount,
        glyphcount,
        state->visattr,
        &analysis,
        bitmap->glyphadvance + bitmap->glyphcount,
        bitmap->glyphoffset + bitmap->glyphcount,
        &abc);
    if (FAILED(hr))
        return -1;

    width = abc.abcA + abc.abcB + abc.abcC;
    if (state->width > 0 && state->posx + width > state->width) {
        breakpos = sg_textbitmap_wrap(bitmap, state, textstart, textcount,
                                      glyphcount, &analysis);
        if (breakpos < 0)
            return -1;

        if (breakpos > 0) {
            /* Rerun the analysis to get an accurate ABC.  */
            glyphcount = state->cluster[breakpos];
            hr = ScriptPlace(
                bitmap->dc,
                &bitmap->cache,
                bitmap->glyph + bitmap->glyphcount,
                glyphcount,
                state->visattr,
                &analysis,
                bitmap->glyphadvance + bitmap->glyphcount,
                bitmap->glyphoffset + bitmap->glyphcount,
                &abc);
            if (FAILED(hr))
                return -1;

            width = abc.abcA + abc.abcB + abc.abcC;
        } else {
            if (state->posx > 0) {
                /* Put the item on the next line.  */
                if (sg_textbitmap_newline(bitmap))
                    return -1;
                state->posx = 0;
                if (width > state->width) {
                    breakpos = sg_textbitmap_wrap(
                        bitmap, state, textstart, textcount,
                        glyphcount, &analysis);
                    if (breakpos < 0)
                        return -1;
                }
            }

            /* If we can't break the line, don't.  */
            if (breakpos == 0)
                breakpos = textcount;
        }
    } else {
        breakpos = textcount;
    }

    run = sg_textbitmap_newrun(bitmap);
    run->analysis = analysis;
    run->posx = state->posx;
    run->glyphoffset = bitmap->glyphcount;
    run->glyphcount = glyphcount;

    line = &bitmap->line[bitmap->linecount - 1];
    line->width = state->posx + width;
    if (metrics->tmAscent > line->ascent)
        line->descent = metrics->tmAscent;
    if (metrics->tmDescent > line->descent)
        line->descent = metrics->tmDescent;
    line->runcount++;

    bitmap->runcount++;
    bitmap->glyphcount += glyphcount;
    state->posx += width;

    if (breakpos < textcount) {
        if (sg_textbitmap_newline(bitmap))
            return -1;
    }

    return breakpos;
}

/* Get the largest number of code units in any item.  */
static int
sg_textbitmap_maxitemlength(struct sg_textbitmap_state *state)
{
    int maxlength, curpos, curitem, itemcount, itemlength, newpos;
    maxlength = 0;
    itemcount = state->itemcount;
    curpos = state->item[0].iCharPos;
    for (curitem = 0; curitem < itemcount; curitem++) {
        newpos = state->item[curitem + 1].iCharPos;
        itemlength = newpos - curpos;
        if (itemlength > maxlength)
            maxlength = itemlength;
        curpos = newpos;
    }
    return maxlength;
}

struct sg_textbitmap *
sg_textbitmap_new_simple(const char *text, size_t textlen,
                         const char *fontname, double fontsize,
                         sg_textalign_t alignment, double width,
                         struct sg_error **err)
{
    struct sg_textbitmap_state state;
    struct sg_textbitmap *bitmap;
    int maxitemlength, curitem, itemcount;
    int textstart, textcount, advance;
    int posy, curline;
    TEXTMETRIC metrics;
    BOOL br;

    memset(&state, 0, sizeof(state));
    bitmap = calloc(1, sizeof(*bitmap));
    if (!bitmap)
        goto error;

    state.width = width > 0.0 ? (int)floor(width + 0.5) : -1;

    /* Convert text to UTF-16.  */
    if (sg_wchar_from_utf8(&state.text, &state.textlen, text, textlen))
        goto error;

    /* Itemize text.  */
    if (sg_textbitmap_itemize(&state))
        goto error;

    /*
     * Allocate buffers for layout.
     * These values (x + x/2 + 16) are recommended by the docs.
     */
    maxitemlength = sg_textbitmap_maxitemlength(&state);
    state.cluster = malloc(sizeof(*state.cluster) * maxitemlength);
    if (!state.cluster)
        goto error;
    if (sg_textbitmap_allocglyphs(
            bitmap, state.textlen + state.textlen / 2 + 16))
        goto error;
    if (sg_textbitmap_allocvisattr(
            &state, maxitemlength + maxitemlength / 2 + 16))
        goto error;

    /* Get the device context handle.  */
    bitmap->dc = CreateCompatibleDC(NULL);
    if (!bitmap->dc)
        goto error;
    bitmap->font = sg_textbitmap_getfont(fontname, fontsize);
    if (!bitmap->font)
        goto error;
    SelectObject(bitmap->dc, bitmap->font);
    br = GetTextMetrics(bitmap->dc, &metrics);
    if (!br)
        goto error;

    /* Place each item.  */
    if (sg_textbitmap_newline(bitmap))
        goto error;
    itemcount = state.itemcount;
    for (curitem = 0; curitem < itemcount; curitem++) {
        textstart = state.item[curitem].iCharPos;
        textcount = state.item[curitem + 1].iCharPos - textstart;
        while (textcount > 0) {
            advance = sg_textbitmap_place(
                bitmap, &state, textstart, textcount,
                state.item[curitem].a, &metrics);
            if (advance < 0)
                goto error;
            textstart += advance;
            textcount -= advance;
        }
    }

    /* Place each line.  */
    posy = 0;
    for (curline = 0; curline < bitmap->linecount; curline++) {
        posy -= bitmap->line[curline].ascent;
        bitmap->line[curline].posy = posy;
        posy -= bitmap->line[curline].descent;
    }

    goto done;

done:
    free(state.text);
    free(state.item);
    free(state.visattr);
    free(state.cluster);
    free(state.logattr);
    return bitmap;

error:
    sg_textbitmap_free(bitmap);
    bitmap = NULL;
    sg_error_nomem(err);
    goto done;
}

void
sg_textbitmap_free(struct sg_textbitmap *bitmap)
{
    if (bitmap->dc)
        DeleteDC(bitmap->dc);
    if (bitmap->font)
        DeleteObject(bitmap->font);
    if (bitmap->cache)
        ScriptFreeCache(&bitmap->cache);
    free(bitmap->line);
    free(bitmap->run);
    free(bitmap->glyph);
    free(bitmap->glyphoffset);
    free(bitmap->glyphadvance);
    free(bitmap);
}

int
sg_textbitmap_getmetrics(struct sg_textbitmap *bitmap,
                         struct sg_textlayout_metrics *metrics,
                         struct sg_error **err)
{
    int i, n;
    struct sg_textbitmap_line *lines;
    int x0, y0, x1, y1, lx0, ly0, lx1, ly1, baseline;
    n = bitmap->linecount;
    lines = bitmap->line;
    if (n > 0) {
        y0 = lines[0].posy - lines[0].descent;
        y1 = lines[0].posy + lines[0].ascent;
        x0 = lines[0].posx;
        x1 = lines[0].posx + lines[0].width;
        baseline = lines[0].posy;
        for (i = 1; i < n; ++i) {
            ly0 = lines[i].posy - lines[i].descent;
            ly1 = lines[i].posy + lines[i].ascent;
            lx0 = lines[i].posx;
            lx1 = lines[i].posx + lines[i].width;
            if (ly0 < y0) y0 = ly0;
            if (lx0 < x0) x0 = lx0;
            if (ly1 > y1) y1 = ly1;
            if (lx1 > x1) x1 = lx1;
        }
    } else {
        x0 = 0;
        y0 = 0;
        x1 = 0;
        y1 = 0;
        baseline = 0;
    }
    metrics->logical.x0 = x0;
    metrics->logical.y0 = y0;
    metrics->logical.x1 = x1;
    metrics->logical.y1 = y1;
    metrics->pixel = metrics->logical;
    metrics->baseline = baseline;
    return 0;
}

static int
sg_textbitmap_renderdc(struct sg_textbitmap *bitmap,
                       struct sg_textpoint offset, int height)
{
    HRESULT hr;
    int posx, posy;
    int curline, endline, currun, endrun;
    struct sg_textbitmap_line *line = bitmap->line;
    struct sg_textbitmap_run *run = bitmap->run;
    WORD *glyph = bitmap->glyph;
    GOFFSET *glyphoffset = bitmap->glyphoffset;
    int *glyphadvance = bitmap->glyphadvance;

    curline = 0;
    endline = bitmap->linecount;
    for (; curline < endline; curline++) {
        posy = height - offset.y - line[curline].ascent - line[curline].posy;
        posx = line[curline].posx;
        currun = line[curline].runoffset;
        endrun = line[curline].runcount + currun;
        for (; currun < endrun; currun++) {
            hr = ScriptTextOut(
                bitmap->dc,
                &bitmap->cache,
                run[currun].posx + posx,
                posy,
                ETO_CLIPPED,
                NULL,
                &run[currun].analysis,
                NULL,
                0,
                glyph + run[currun].glyphoffset,
                run[currun].glyphcount,
                glyphadvance + run[currun].glyphoffset,
                NULL,
                glyphoffset + run[currun].glyphoffset);
            if (FAILED(hr))
                return -1;
        }
    }

    return 0;
}

static int
sg_textbitmap_createbitmap(HDC dc, int width, int height,
                           HBITMAP *hbit, void **data)
{
    HBITMAP hbitmap;
    BITMAPINFO bmi;
    BITMAPINFOHEADER *hdr;
    void *ptr;

    hdr = &bmi.bmiHeader;
    hdr->biSize = sizeof(*hdr);
    hdr->biWidth = width;
    hdr->biHeight = -height;
    hdr->biPlanes = 1;
    hdr->biBitCount = 32;
    hdr->biCompression = BI_RGB;
    hdr->biSizeImage = 0;
    hdr->biXPelsPerMeter = 0;
    hdr->biClrUsed = 0;
    hdr->biClrImportant = 0;
    hbitmap = CreateDIBSection(dc, &bmi, DIB_RGB_COLORS, &ptr, NULL, 0);
    if (!hbitmap)
        return -1;

    *hbit = hbitmap;
    *data = ptr;
    return 0;
}

static void
sg_textbitmap_copypixels(void *dest, size_t destrowbytes,
                         const void *src, size_t srcrowbytes,
                         int width, int height)
{
    unsigned char *dptr = dest;
    const unsigned char *sptr = src;
    int x, y;

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++)
            dptr[x] = sptr[x * 4];
        dptr += destrowbytes;
        sptr += srcrowbytes;
    }
}

int
sg_textbitmap_render(struct sg_textbitmap *bitmap,
                     struct sg_pixbuf *pixbuf,
                     struct sg_textpoint offset,
                     struct sg_error **err)
{
    HDC dc;
    HBITMAP hbitmap;
    void *bitmapdata;
    int width = pixbuf->pwidth;
    int height = pixbuf->pheight;

    dc = bitmap->dc;
    SelectObject(dc, bitmap->font);
    if (sg_textbitmap_createbitmap(dc, width, height, &hbitmap, &bitmapdata))
        goto error;
    SelectObject(dc, hbitmap);
    SetBkColor(dc, RGB(0, 0, 0));
    SetTextColor(dc, RGB(255, 255, 255));
    if (sg_textbitmap_renderdc(bitmap, offset, height))
        goto error;
    sg_textbitmap_copypixels(pixbuf->data, pixbuf->rowbytes,
                             bitmapdata, width * 4, width, height);
    DeleteObject(hbitmap);
    return 0;

error:
    if (hbitmap)
        DeleteObject(hbitmap);
    sg_error_nomem(err);
    return -1;
}
