#include "pixbuf.h"
#include "type_impl.h"
#include <Windows.h>
#include <usp10.h>

#pragma comment(lib, "Usp10.lib")

static int sg_uniscribe_initted;
static SCRIPT_DIGITSUBSTITUTE sg_uniscribe_psds;
static SCRIPT_CACHE sg_uniscribe_cache;
static HDC sg_uniscribe_dc;

struct sg_layout_impl {
    HDC dc;

    int ascent;

    /* Bitmap info */
    HBITMAP bitmap;
    void *data;

    /* Item info */
    int nitems;
    SCRIPT_ITEM *items;
    int *itemglyphs;
    int *itemoffsets;

    /* Glyph info */
    int nglyphs;
    WORD *glyphs;
    GOFFSET *offsets;
    int *advance;
};

void
sg_layout_impl_free(struct sg_layout_impl *li)
{
    /* Unimplemented */
}

static struct sg_layout_impl *
sg_layout_getimpl(struct sg_layout *lp)
{
    struct sg_layout_impl *li;
    li = lp->impl;
    if (!li) {
        li = calloc(sizeof(*li), 1);
        if (!li)
            abort();
        lp->impl = li;
    }
    return li;
}

static void
sg_layout_initdc(struct sg_layout_impl *li)
{
    LOGFONTW f;
    HFONT fh;
    if (li->dc)
        return;
    li->dc = CreateCompatibleDC(NULL);
    if (!li->dc)
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
    SelectObject(li->dc, fh);
}

static void
sg_layout_initbitmap(struct sg_layout_impl *li, int w, int h)
{
    HBITMAP hbit;
    BITMAPINFO bmi;
    BITMAPINFOHEADER *hdr;
    void *bptr;

    sg_layout_initdc(li);

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

    hbit = CreateDIBSection(li->dc, &bmi, DIB_RGB_COLORS, &bptr, NULL, 0);
    if (!hbit)
        abort();
    SelectObject(li->dc, hbit);
    li->bitmap = hbit;
    li->data = bptr;
}

static void
sg_layout_uniscribe_init(void)
{
    HRESULT hr;
    hr = ScriptRecordDigitSubstitution(LOCALE_USER_DEFAULT, &sg_uniscribe_psds);
    if (FAILED(hr))
        abort();
}

void
sg_layout_calcbounds(struct sg_layout *lp, struct sg_layout_bounds *b)
{
    struct sg_layout_impl *li;
    HDC dc;
    wchar_t *wtext;
    int r, wtextlen;
    HRESULT hr;
    SCRIPT_ITEM *items; /* [nitems+1] */
    int *itemglyphs; /* [nitems+1] glyph ranges for each item */
    int *itemoffsets; /* [nitems] offsets for each item */
    int nitems, aitems, curitem;
    WORD *glyphs; /* [nglyphs] Glyphs */
    WORD *clusters; /* [wtextlen] glyph index for each character */
    GOFFSET *offsets; /* [nglyphs] locations for each glyph */
    int nglyphs, aglyphs, curglyph;
    SCRIPT_VISATTR *visattr; /* [nglyphs] visual attributes for each glyph */
    int *advance; /* [nglyphs] advance of each glyph */
    ABC abc;
    TEXTMETRIC metrics;
    BOOL br;
    int curpos;

    li = sg_layout_getimpl(lp);

    r = MultiByteToWideChar(CP_UTF8, 0, lp->text, lp->textlen, NULL, 0);
    if (!r)
        abort();
    wtextlen = r;
    wtext = malloc(sizeof(wchar_t) * wtextlen);
    r = MultiByteToWideChar(CP_UTF8, 0, lp->text, lp->textlen, wtext, wtextlen);
    if (!r)
        abort();

    if (!sg_uniscribe_initted)
        sg_layout_uniscribe_init();
    
    sg_layout_initdc(li);
    dc = li->dc;

    br = GetTextMetrics(dc, &metrics);
    if (!br)
        abort();

    aitems = wtextlen / 16 + 4; /* This is a guess */
alloc_items:
    /* ScriptItemize documentation says buffer should have this size.
       It doesn't explain why.  */
    items = malloc(sizeof(*items) * aitems + 1);
    if (!items)
        abort();
    hr = ScriptItemize(wtext, wtextlen, aitems, NULL, NULL, items, &nitems);
    if (FAILED(hr)) {
        if (hr == E_OUTOFMEMORY && aitems < wtextlen) {
            free(items);
            aitems *= 2;
            if (aitems > wtextlen)
                aitems = wtextlen;
            goto alloc_items;
        }
        abort();
    }

    curitem = 0;
    curglyph = 0;
    glyphs = NULL;
    visattr = NULL;
    itemglyphs = malloc(sizeof(*itemglyphs) * (nitems + 1));
    clusters = malloc(sizeof(*clusters) * wtextlen);
    if (!clusters)
        abort();
    aglyphs = wtextlen + wtextlen / 2 + 16; /* Recommended by docs */
alloc_glyphs:
    glyphs = realloc(glyphs, sizeof(*glyphs) * aglyphs);
    if (!glyphs)
        abort();
    visattr = realloc(visattr, sizeof(*visattr) * aglyphs);
    if (!visattr)
        abort();
    for (; curitem < nitems; ++curitem) {
        hr = ScriptShape(
            dc,
            &sg_uniscribe_cache,
            wtext + items[curitem].iCharPos,
            items[curitem + 1].iCharPos - items[curitem].iCharPos,
            aglyphs - curglyph,
            &items[curitem].a,
            glyphs + curglyph,
            clusters + items[curitem].iCharPos,
            visattr + curglyph,
            &nglyphs);
        if (FAILED(hr)) {
            if (hr == E_OUTOFMEMORY) {
                aglyphs *= 2;
                goto alloc_glyphs;
            }
            abort();
        }
        itemglyphs[curitem] = curglyph;
        curglyph += nglyphs;
    }
    itemglyphs[curitem] = curglyph;
    nglyphs = curglyph;

    curpos = 0;
    itemoffsets = malloc(sizeof(*itemoffsets) * nitems);
    if (!itemoffsets)
        abort();
    offsets = malloc(sizeof(*offsets) * nglyphs);
    if (!offsets)
        abort();
    advance = malloc(sizeof(*advance) * nglyphs);
    if (!advance)
        abort();
    curitem = 0;
    for (; curitem < nitems; ++curitem) {
        itemoffsets[curitem] = curpos;
        hr = ScriptPlace(
            dc,
            &sg_uniscribe_cache,
            glyphs + itemglyphs[curitem],
            itemglyphs[curitem + 1] - itemglyphs[curitem],
            visattr + itemglyphs[curitem],
            &items[curitem].a,
            advance + itemglyphs[curitem],
            offsets + itemglyphs[curitem],
            &abc);
        curpos += abc.abcA + abc.abcB + abc.abcC;
        if (FAILED(hr))
            abort();
    }

    li->nitems = nitems;
    li->items = items;
    li->itemglyphs = itemglyphs;
    li->itemoffsets = itemoffsets;
    li->nglyphs = nglyphs;
    li->glyphs = glyphs;
    li->offsets = offsets;
    li->advance = advance;
    li->ascent = metrics.tmAscent;
    b->x = 0;
    b->y = 0;
    b->ibounds.x = 0;
    b->ibounds.y = -metrics.tmDescent;
    b->ibounds.width = curpos;
    b->ibounds.height = metrics.tmHeight;
}

void
sg_layout_render(struct sg_layout *lp, struct sg_pixbuf *pbuf,
                 int xoff, int yoff)
{
    struct sg_layout_impl *li;
    HDC dc;
    int nitems, curitem;
    SCRIPT_ITEM *items;
    int *itemglyphs, *itemoffsets;
    WORD *glyphs;
    int *advance;
    GOFFSET *offsets;
    HRESULT hr;
    unsigned char *sptr, *dptr;
    unsigned irb, orb;
    int x, y, w, h;

    li = lp->impl;
    w = pbuf->iwidth;
    h = pbuf->iheight;
    sg_layout_initbitmap(li, w, h);
    dc = li->dc;
    nitems = li->nitems;
    items = li->items;
    itemglyphs = li->itemglyphs;
    itemoffsets = li->itemoffsets;
    glyphs = li->glyphs;
    advance = li->advance;
    offsets = li->offsets;

    for (curitem = 0; curitem < nitems; ++curitem) {
        hr = ScriptTextOut(
            dc,
            &sg_uniscribe_cache,
            itemoffsets[curitem],
            0,
            ETO_CLIPPED,
            NULL,
            &items[curitem].a,
            NULL,
            0,
            glyphs + itemglyphs[curitem],
            itemglyphs[curitem + 1] - itemglyphs[curitem],
            advance + itemglyphs[curitem],
            NULL,
            offsets + itemglyphs[curitem]);
        if (FAILED(hr))
            abort();
    }

    sptr = li->data;
    dptr = (unsigned char *) pbuf->data +
        xoff + (pbuf->pheight - yoff - li->ascent) * pbuf->rowbytes;
    irb = w * 4;
    orb = pbuf->rowbytes;
    for (y = 0; y < h; ++y) {
        for (x = 0; x < w; ++x)
            dptr[x] = ~sptr[x*4+1];
        sptr += irb;
        dptr += orb;
    }
}
