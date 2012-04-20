#include "defs.h"

#include "pixbuf.h"
#include "type_impl.h"
#include <Windows.h>
#include <usp10.h>
#include <math.h>
#include <limits.h>

#pragma comment(lib, "Usp10.lib")

static int
round_up_pow2(unsigned x)
{
    x -= 1;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    return x + 1;
}

static int sg_uniscribe_initted;
static SCRIPT_DIGITSUBSTITUTE sg_uniscribe_psds;
static HDC sg_uniscribe_dc;

/* All coordinates are Windows pixel coordinates,
   i.e., the Y-axis is flipped compared to OpenGL.  */

struct sg_layout_line {
    int yoff, xoff, width;
    int ascent, descent;
    int item_off, item_count;
};

struct sg_layout_item {
    SCRIPT_ANALYSIS analysis;
    int xoff;
    int glyph_off, glyph_count;
};

struct sg_layout_impl {
    struct sg_layout *lp;
    HDC dc;

    /* Line info */
    int nlines;
    struct sg_layout_line *lines;

    /* Item info */
    int nitems;
    struct sg_layout_item *items;

    /* Glyph info */
    int nglyphs;
    WORD *glyphs;
    GOFFSET *goffsets; /* Locations for each glyph */
    int *gadvances; /* Advance of each glyph */
};

static void
sg_layout_setfont(HDC dc, struct sg_layout *lp)
{
    LOGFONTW f;
    HFONT fh;

    f.lfHeight = -(long) floorf(lp->size + 0.5f);
    // f.lfHeight = -16;
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
    f.lfQuality = CLEARTYPE_QUALITY;
    f.lfPitchAndFamily = DEFAULT_PITCH;
    wcscpy(f.lfFaceName, L"Arial");
    fh = CreateFontIndirectW(&f);
    if (!fh)
        abort();
    SelectObject(dc, fh);
    DeleteObject(fh);
}

static void
sg_layout_uniscribe_init(void)
{
    HRESULT hr;
    hr = ScriptRecordDigitSubstitution(LOCALE_USER_DEFAULT, &sg_uniscribe_psds);
    if (FAILED(hr))
        abort();
}

struct sg_layout_impl *
sg_layout_impl_new(struct sg_layout *lp)
{
    struct sg_layout_impl *li = NULL;
    int r, i, j, len, cstart, clen, maxg, nalloc;
    int curx, cury, nx, adv, width, c, wrem;
    HRESULT hr;
    HDC dc;
    ABC abc;
    TEXTMETRIC metrics;
    BOOL br;
    SCRIPT_CACHE cache = NULL;
    SCRIPT_ANALYSIS analysis;

    /* UTF-16 text */
    int wtextlen;
    wchar_t *wtext = NULL;

    /* line arrays */
    int nlines, alines, curline;
    struct sg_layout_line *lines, *newlines;

    /* item arrays */
    int aitems, curitem, firstitem, nsitems, cursitem, asitems;
    SCRIPT_ITEM *sitems = NULL;
    struct sg_layout_item *items = NULL, *newitems;
    int maxilen; /* Maximum length, in characters, of any item */

    /* glyph arrays */
    int nglyphs, aglyphs, curglyph;
    WORD *glyphs = NULL, *newglyphs = NULL, *clusters = NULL;
    int *gadvances = NULL, *newgadvances = NULL;
    GOFFSET *goffsets = NULL, *newgoffsets = NULL;

    /* glyph visattr (temporary) */
    int avisattr;
    SCRIPT_VISATTR *visattr = NULL;

    /* character logattr (temporary) */
    int alogattr;
    SCRIPT_LOGATTR *logattr = NULL;

    /***** Convert text to UTF-16 *****/
    r = MultiByteToWideChar(CP_UTF8, 0, lp->text, lp->textlen, NULL, 0);
    if (!r) goto error;
    wtextlen = r;
    wtext = malloc(sizeof(wchar_t) * wtextlen);
    r = MultiByteToWideChar(CP_UTF8, 0, lp->text, lp->textlen, wtext, wtextlen);
    if (!r) goto error;

    /***** Break text into items *****/
    /* This is a guess, given by the ScriptItemize documentation */
    asitems = wtextlen / 16 + 4;
alloc_items:
    sitems = malloc(sizeof(*sitems) * (asitems + 1));
    if (!sitems)
        abort();
    hr = ScriptItemize(wtext, wtextlen, asitems, NULL, NULL, sitems, &nsitems);
    if (FAILED(hr)) {
        if (hr == E_OUTOFMEMORY && asitems < wtextlen) {
            free(sitems);
            asitems *= 2;
            if (asitems > wtextlen)
                asitems = wtextlen;
            goto alloc_items;
        }
        goto error;
    }

    /* Find length of longest item */
    maxilen = 0;
    i = sitems[0].iCharPos;
    for (cursitem = 0; cursitem < nsitems; ++cursitem) {
        j = sitems[cursitem + 1].iCharPos;
        len = j - i;
        i = j;
        if (len > maxilen)
            maxilen = len;
    }

    /* Get the device context handle */
    dc = CreateCompatibleDC(NULL);
    sg_layout_setfont(dc, lp);

    /***** Place each item *****/
    /* Creates new item array, and all the glyph arrays (glyphs, offsets, advances) */
    aitems = nsitems;
    items = malloc(sizeof(*items) * aitems);
    if (!items) goto error;

    clusters = malloc(sizeof(*clusters) * maxilen);
    if (!clusters) goto error;

    aglyphs = wtextlen + wtextlen / 2 + 16; /* Recommended by docs */
    glyphs = malloc(sizeof(*glyphs) * aglyphs);
    if (!glyphs) goto error;
    gadvances = malloc(sizeof(*gadvances) * aglyphs);
    if (!gadvances) goto error;
    goffsets = malloc(sizeof(*goffsets) * aglyphs);
    if (!goffsets) goto error;

    avisattr = maxilen + maxilen / 2 + 16;
    visattr = malloc(sizeof(*visattr) * avisattr);
    if (!visattr) goto error;

    alines = 1;
    nlines = 0;
    curline = 0;
    lines = malloc(sizeof(*lines) * alines);
    if (!lines) goto error;

    width = (lp->width >= 0) ? (int) floorf(lp->width + 0.5f) : INT_MAX;
    curglyph = 0;
    curx = 0;
    cury = 0;
    br = GetTextMetrics(dc, &metrics);
    if (!br) goto error;

    curitem = 0;
    curline = 0;
    firstitem = 0;
    for (cursitem = 0; cursitem < nsitems; ++cursitem) {
        cstart = sitems[cursitem].iCharPos;
        clen = sitems[cursitem + 1].iCharPos - cstart;
        analysis = sitems[cursitem].a;

    shape_again:
        if (curitem >= aitems) {
            nalloc = aitems * 2;
            newitems = realloc(items, sizeof(*items) * nalloc);
            if (!newitems) goto error;
            items = newitems;
            aitems = nalloc;
        }
        maxg = aglyphs - curglyph;
        if (avisattr < maxg)
            maxg = avisattr;
        hr = ScriptShape(
            dc,
            &cache,
            wtext + cstart,
            clen,
            maxg,
            &analysis,
            glyphs + curglyph,
            clusters,
            visattr,
            &nglyphs);
        if (FAILED(hr)) {
            if (hr == E_OUTOFMEMORY) {
                if (maxg == aglyphs - curglyph) {
                    nalloc = aglyphs * 2;

                    newglyphs = realloc(glyphs, sizeof(*glyphs) * nalloc);
                    if (!newglyphs) goto error;
                    glyphs = newglyphs;

                    newgadvances = realloc(gadvances, sizeof(*gadvances) * nalloc);
                    if (!newgadvances) goto error;
                    gadvances = newgadvances;

                    newgoffsets = realloc(goffsets, sizeof(*goffsets) * nalloc);
                    if (!newgoffsets) goto error;
                    goffsets = newgoffsets;

                    aglyphs = nalloc;
                }
                if (maxg == avisattr) {
                    nalloc = avisattr * 2;
                    free(visattr);
                    visattr = malloc(sizeof(*visattr) * avisattr);
                    if (!visattr) goto error;
                    avisattr = nalloc;
                }
                goto shape_again;
            }
            goto error;
        }

        hr = ScriptPlace(
            dc,
            &cache,
            glyphs + curglyph,
            nglyphs,
            visattr,
            &analysis,
            gadvances + curglyph,
            goffsets + curglyph,
            &abc);
        if (FAILED(hr))
            goto error;

        nx = curx + abc.abcA + abc.abcB + abc.abcC;
        if (nx > width) {
            /* Find the first glyph past the limit */
            /* i = first glyph */
            wrem = width - curx;
            for (i = 0; i < nglyphs; ++i) {
                adv = gadvances[curglyph + i];
                if (adv > wrem)
                    break;
                wrem -= adv;
            }

            /* Find the first character corresponding to
               a cluster containing the glyph after the break */
            for (j = 0; j < clen; ++j) {
                c = clusters[j];
                if (c >= i) {
                    if (c > i && j > 0) {
                        j--;
                        c = clusters[j];
                        while (j > 0 && clusters[j - 1] == c)
                            j--;
                    }
                    break;
                }
            }

            /* Analyze characters in item */
            if (logattr && clen > alogattr) {
                free(logattr);
                logattr = NULL;
            }
            if (!logattr) {
                alogattr = round_up_pow2(clen);
                logattr = malloc(sizeof(*logattr) * alogattr);
                if (!logattr) goto error;
            }
            hr = ScriptBreak(
                wtext + cstart,
                clen,
                &analysis,
                logattr);

            /* Scan for a break character no later than j */
            i = (j >= clen) ? clen - 1 : j;
            for (; i >= 0; --i) {
                if (logattr[i].fWhiteSpace) {
                    j = i + 1;
                    break;
                } else if (logattr[i].fSoftBreak) {
                    j = i;
                    break;
                }
            }

            /* i = # of characters in this line
               j = relative offset of first character in next line */

            if (i > 0) {
                /* Rerun the analysis to get an accurate ABC */
                nglyphs = clusters[i];
                hr = ScriptPlace(
                    dc,
                    &cache,
                    glyphs + curglyph,
                    nglyphs,
                    visattr,
                    &analysis,
                    gadvances + curglyph,
                    goffsets + curglyph,
                    &abc);
                if (FAILED(hr))
                    goto error;
                nx = curx + abc.abcA + abc.abcB + abc.abcC;
                cstart += i;
                clen -= i;
            } else {
                /* If we can't break the line, don't */
                cstart = 0;
                clen = 0;
            }
        } else {
            cstart = 0;
            clen = 0;
        }
        items[curitem].analysis = sitems[curitem].a;
        items[curitem].xoff = curx;
        items[curitem].glyph_off = curglyph;
        items[curitem].glyph_count = nglyphs;
        curglyph += nglyphs;
        curx = nx;
        curitem++;

        if (clen > 0) {
            if (curline >= alines) {
                nalloc = alines * 2;
                newlines = realloc(lines, sizeof(*lines) * nalloc);
                if (!newlines) goto error;
                lines = newlines;
                alines = nalloc;
            }
            lines[curline].xoff = 0;
            lines[curline].yoff = metrics.tmAscent + cury;
            lines[curline].width = curx;
            lines[curline].ascent = metrics.tmAscent;
            lines[curline].descent = metrics.tmDescent;
            lines[curline].item_off = firstitem;
            lines[curline].item_count = curitem - firstitem;
            curline++;
            firstitem = curitem;
            cury += metrics.tmAscent + metrics.tmDescent;
            curx = 0;
            goto shape_again;
        }
    }
    lines[curline].xoff = 0;
    lines[curline].yoff = metrics.tmAscent + cury;
    lines[curline].width = curx;
    lines[curline].ascent = metrics.tmAscent;
    lines[curline].descent = metrics.tmDescent;
    lines[curline].item_off = firstitem;
    lines[curline].item_count = curitem - firstitem;
    curline++;
    nglyphs = curglyph;

    li = malloc(sizeof(*li));
    if (!li) goto error;

    li->lp = lp;
    li->dc = dc;
    li->nlines = curline;
    li->lines = lines;
    li->nitems = nsitems;
    li->items = items;
    li->nglyphs = nglyphs;
    li->glyphs = glyphs;
    li->goffsets = goffsets;
    li->gadvances = gadvances;

    if (cache)
        ScriptFreeCache(&cache);

    free(wtext);
    free(sitems);
    free(clusters);
    free(visattr);
    free(logattr);

    return li;

error:
    abort(); /* Since the caller can't handle errors from here yet */

    if (cache)
        ScriptFreeCache(&cache);

    if (dc)
        DeleteDC(dc);

    free(lines);

    free(items);
    free(glyphs);
    free(goffsets);
    free(gadvances);

    free(wtext);
    free(items);
    free(clusters);
    free(visattr);
    free(logattr);

    return NULL;
}

void
sg_layout_impl_free(struct sg_layout_impl *li)
{
    if (li->dc)
        DeleteDC(li->dc);
    free(li->items);
    free(li->glyphs);
    free(li->goffsets);
    free(li->gadvances);
    free(li);
}

void
sg_layout_impl_calcbounds(struct sg_layout_impl *li,
                          struct sg_layout_bounds *b)
{
    int i, nlines;
    struct sg_layout_line *lines;
    int miny, minx, maxy, maxx, lminy, lminx, lmaxy, lmaxx;
    int x0, y0;
    nlines = li->nlines;
    lines = li->lines;
    if (nlines > 0) {
        miny = - lines[0].yoff - lines[0].descent;
        maxy = - lines[0].yoff + lines[0].ascent;
        minx = lines[0].xoff;
        maxx = lines[0].xoff + lines[0].width;
        x0 = lines[0].xoff;
        y0 = lines[0].yoff;
        for (i = 1; i < nlines; ++i) {
            lminy = - lines[i].yoff - lines[i].descent;
            lmaxy = - lines[i].yoff + lines[i].ascent;
            lminx = lines[i].xoff;
            lmaxx = lines[i].xoff + lines[i].width;
            if (lminy < miny) miny = lminy;
            if (lminx < minx) minx = lminx;
            if (lmaxy > maxy) maxy = lmaxy;
            if (lmaxx > maxx) maxx = lmaxx;
        }
    } else {
        miny = 0;
        minx = 0;
        maxy = 0;
        maxx = 0;
        x0 = 0;
        y0 = 0;
    }
    b->x = x0;
    b->y = y0;
    b->logical.x = minx;
    b->logical.y = miny;
    b->logical.width = maxx - minx;
    b->logical.height = maxy - miny;
    b->pixel = b->logical;
}

void
sg_layout_impl_render(struct sg_layout_impl *li, struct sg_pixbuf *pbuf,
                      int xoff, int yoff)
{
    HDC dc;
    HRESULT hr;
    SCRIPT_CACHE cache = NULL;

    /* Bitmap info */
    HBITMAP hbit;
    BITMAPINFO bmi;
    BITMAPINFOHEADER *hdr;
    void *bptr;

    /* Glyph / item / line info */
    int ypos, curline, endline, curitem, enditem;
    struct sg_layout_line *lines;
    struct sg_layout_item *items;
    WORD *glyphs;
    GOFFSET *goffsets;
    int *gadvances;

    /* Pixel buffer copy state */
    unsigned char *sptr, *dptr;
    unsigned irb, orb;
    int x, y, w, h;

    dc = li->dc;
    sg_layout_setfont(dc, li->lp);
    w = pbuf->pwidth;
    h = pbuf->pheight;

    /***** Create bitmap *****/

    hdr = &bmi.bmiHeader;
    hdr->biSize = sizeof(*hdr);
    hdr->biWidth = w;
    hdr->biHeight = -h;
    hdr->biPlanes = 1;
    hdr->biBitCount = 32;
    hdr->biCompression = BI_RGB;
    hdr->biSizeImage = 0;
    hdr->biXPelsPerMeter = 0;
    hdr->biClrUsed = 0;
    hdr->biClrImportant = 0;

    hbit = CreateDIBSection(dc, &bmi, DIB_RGB_COLORS, &bptr, NULL, 0);
    if (!hbit)
        goto error;
    SelectObject(dc, hbit);

    /***** Draw glyphs *****/

    lines = li->lines;
    items = li->items;
    items = li->items;
    glyphs = li->glyphs;
    gadvances = li->gadvances;
    goffsets = li->goffsets;

    curline = 0;
    endline = li->nlines;
    for (; curline < endline; ++curline) {
        ypos = lines[curline].yoff - lines[curline].ascent + (h - yoff);
        curitem = lines[curline].item_off;
        enditem = curitem + lines[curline].item_count;
        for (; curitem < enditem; ++curitem) {
            hr = ScriptTextOut(
                dc,
                &cache,
                items[curitem].xoff + xoff,
                ypos,
                ETO_CLIPPED,
                NULL,
                &items[curitem].analysis,
                NULL,
                0,
                glyphs + items[curitem].glyph_off,
                items[curitem].glyph_count,
                gadvances + items[curitem].glyph_off,
                NULL,
                goffsets + items[curitem].glyph_off);
            if (FAILED(hr))
                goto error;
        }
    }

    /***** Copy bitmap data to pixel buffer *****/

    sptr = bptr;
    dptr = (unsigned char *) pbuf->data;
    irb = w * 4;
    orb = pbuf->rowbytes;
    for (y = 0; y < h; ++y) {
        for (x = 0; x < w; ++x)
            dptr[x] = (~sptr[x*4+0]);
        sptr += irb;
        dptr += orb;
    }

    if (cache)
        ScriptFreeCache(&cache);
    DeleteObject(hbit);
    return;

error:
    if (cache)
        ScriptFreeCache(&cache);
    DeleteObject(hbit);
}
