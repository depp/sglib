#include "defs.h"

#include "pixbuf.h"
#include "type_impl.h"
#include <Windows.h>
#include <usp10.h>

#pragma comment(lib, "Usp10.lib")

static int sg_uniscribe_initted;
static SCRIPT_DIGITSUBSTITUTE sg_uniscribe_psds;
static SCRIPT_CACHE sg_uniscribe_cache;
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

static HDC
sg_layout_initdc(void)
{
    HDC dc;
    LOGFONTW f;
    HFONT fh;

    dc = CreateCompatibleDC(NULL);
    if (!dc)
        abort();

    f.lfHeight = -16;
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

    return dc;
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
    int r, i, j, len, cstart, clen, maxg, nalloc, curx;
    HRESULT hr;
    HDC dc;
    ABC abc;
    TEXTMETRIC metrics;
    BOOL br;

    /* UTF-16 text */
    int wtextlen;
    wchar_t *wtext = NULL;

    /* line arrays */
    struct sg_layout_line *lines; /* only one for now... */

    /* item arrays */
    int aitems, nitems, curitem;
    SCRIPT_ITEM *sitems = NULL;
    struct sg_layout_item *items = NULL;
    int maxilen; /* Maximum length, in characters, of any item */

    /* glyph arrays */
    int nglyphs, aglyphs, curglyph;
    WORD *glyphs = NULL, *newglyphs = NULL, *clusters = NULL;
    int *gadvances = NULL, *newgadvances = NULL;
    GOFFSET *goffsets = NULL, *newgoffsets = NULL;

    /* glyph visattr (temporary) */
    int avisattr;
    SCRIPT_VISATTR *visattr = NULL;

    /***** Convert text to UTF-16 *****/
    r = MultiByteToWideChar(CP_UTF8, 0, lp->text, lp->textlen, NULL, 0);
    if (!r) goto error;
    wtextlen = r;
    wtext = malloc(sizeof(wchar_t) * wtextlen);
    r = MultiByteToWideChar(CP_UTF8, 0, lp->text, lp->textlen, wtext, wtextlen);
    if (!r) goto error;

    /***** Break text into items *****/
    /* This is a guess, given by the ScriptItemize documentation */
    aitems = wtextlen / 16 + 4;
alloc_items:
    sitems = malloc(sizeof(*sitems) * (aitems + 1));
    if (!sitems)
        abort();
    hr = ScriptItemize(wtext, wtextlen, aitems, NULL, NULL, sitems, &nitems);
    if (FAILED(hr)) {
        if (hr == E_OUTOFMEMORY && aitems < wtextlen) {
            free(sitems);
            aitems *= 2;
            if (aitems > wtextlen)
                aitems = wtextlen;
            goto alloc_items;
        }
        goto error;
    }

    /* Find length of longest item */
    maxilen = 0;
    i = sitems[0].iCharPos;
    for (curitem = 0; curitem < nitems; ++curitem) {
        j = sitems[curitem + 1].iCharPos;
        len = j - i;
        i = j;
        if (len > maxilen)
            maxilen = len;
    }

    /* Get the device context handle */
    dc = sg_layout_initdc();

    /***** Place each item *****/
    /* Creates new item array, and all the glyph arrays (glyphs, offsets, advances) */
    items = malloc(sizeof(*items) * nitems);
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

    lines = malloc(sizeof(*lines));
    if (!lines) goto error;

    curglyph = 0;
    curx = 0;
    for (curitem = 0; curitem < nitems; ++curitem) {
        cstart = sitems[curitem].iCharPos;
        clen = sitems[curitem + 1].iCharPos - cstart;

    shape_again:
        maxg = aglyphs - curglyph;
        if (avisattr < maxg)
            maxg = avisattr;
        hr = ScriptShape(
            dc,
            &sg_uniscribe_cache,
            wtext + cstart,
            clen,
            maxg,
            &sitems[curitem].a,
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
            &sg_uniscribe_cache,
            glyphs + curglyph,
            nglyphs,
            visattr,
            &sitems[curitem].a,
            gadvances + curglyph,
            goffsets + curglyph,
            &abc);
        if (FAILED(hr))
            goto error;

        items[curitem].analysis = sitems[curitem].a;
        items[curitem].xoff = curx;
        items[curitem].glyph_off = curglyph;
        items[curitem].glyph_count = nglyphs;

        curglyph += nglyphs;
        curx += abc.abcA + abc.abcB + abc.abcC;
    }
    nglyphs = curglyph;

    br = GetTextMetrics(dc, &metrics);
    if (!br) goto error;
    lines[0].yoff = metrics.tmAscent;
    lines[0].xoff = 0;
    lines[0].width = curx;
    lines[0].ascent = metrics.tmAscent;
    lines[0].descent = metrics.tmDescent;
    lines[0].item_off = 0;
    lines[0].item_count = nitems;

    li = malloc(sizeof(*li));
    if (!li) goto error;

    li->dc = dc;
    li->nlines = 1;
    li->lines = lines;
    li->nitems = nitems;
    li->items = items;
    li->nglyphs = nglyphs;
    li->glyphs = glyphs;
    li->goffsets = goffsets;
    li->gadvances = gadvances;

    free(wtext);
    free(sitems);
    free(clusters);
    free(visattr);

    return li;

error:
    abort(); /* Since the caller can't handle errors from here yet */

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
                &sg_uniscribe_cache,
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

    DeleteObject(hbit);
    return;

error:
    abort();
    DeleteObject(hbit);
}
