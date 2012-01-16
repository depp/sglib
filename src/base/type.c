#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "opengl.h"
#include "pixbuf.h"
#include "type.h"
#include <stdlib.h>
#include <string.h>

#if defined(HAVE_CORETEXT)
#include <CoreText/CoreText.h>
#include <CoreGraphics/CoreGraphics.h>
#elif defined(HAVE_PANGOCAIRO)
#include <pango/pango.h>
#include <pango/pangocairo.h>
#endif

struct sg_layout {
    unsigned refcount;
    char *text;
    unsigned textlen;

    GLuint texnum;

    float vx0, vx1, vy0, vy1;
    float tx0, tx1, ty0, ty1;

#if defined(HAVE_CORETEXT)
    CTLineRef ct_line;
#elif defined(HAVE_PANGOCAIRO)
    PangoLayout *pango_layout;
#endif
};

struct sg_layout_rect {
    int x, y, width, height;
};

struct sg_layout_bounds {
    int x, y;
    struct sg_layout_rect ibounds, lbounds;
};

/* These defs are to give us more informative stack traces.  */
#if defined(HAVE_CORETEXT)
#define sg_layout_calcbounds sg_layoutct_calcbounds
#define sg_layout_render sg_layoutct_render
#elif defined(HAVE_PANGOCAIRO)
#define sg_layout_calcbounds sg_layoutpc_calcbounds
#define sg_layout_render sg_layoutpc_render
#endif

/* Calculate the bounds of the given layout.  */
static void
sg_layout_calcbounds(struct sg_layout *lp, struct sg_layout_bounds *b);

/* Render the layout at the given location in a pixel buffer.  */
static void
sg_layout_render(struct sg_layout *lp, struct sg_pixbuf *pbuf,
                 int xoff, int yoff);

struct sg_layout *
sg_layout_new(void)
{
    struct sg_layout *lp;
    lp = malloc(sizeof(*lp));
    if (!lp)
        abort();
    lp->refcount = 1;
    lp->text = NULL;
    lp->textlen = 0;
    lp->texnum = 0;

#if defined(HAVE_CORETEXT)
    lp->ct_line = NULL;
#elif defined(HAVE_PANGOCAIRO)
    lp->pango_layout = NULL;
#endif

    return lp;
}

static void
sg_layout_free(struct sg_layout *lp)
{
    if (lp->texnum)
        glDeleteTextures(1, &lp->texnum);
    free(lp);
}

void
sg_layout_incref(struct sg_layout *lp)
{
    if (lp)
        lp->refcount++;
}

void
sg_layout_decref(struct sg_layout *lp)
{
    if (lp && --lp->refcount == 0)
        sg_layout_free(lp);
}

void
sg_layout_settext(struct sg_layout *lp, const char *text, unsigned length)
{
    char *tp;
    tp = malloc(length + 1);
    if (!tp)
        abort();
    memcpy(tp, text, length);
    tp[length] = '\0';
    if (lp->text)
        free(lp->text);
    lp->text = tp;
    lp->textlen = length;
}

static void
sg_layout_load(struct sg_layout *lp)
{
    struct sg_layout_bounds b;
    struct sg_pixbuf pbuf;
    struct sg_error *err = NULL;
    int r, xoff, yoff;
    float xscale, yscale;

    sg_layout_calcbounds(lp, &b);

    lp->vx0 = b.ibounds.x - b.x;
    lp->vx1 = b.ibounds.x - b.x + b.ibounds.width;
    lp->vy0 = b.ibounds.y - b.y;
    lp->vy1 = b.ibounds.y - b.y + b.ibounds.height;

    sg_pixbuf_init(&pbuf);
    r = sg_pixbuf_set(&pbuf, SG_Y, b.ibounds.width, b.ibounds.height, &err);
    if (r)
        abort();
    r = sg_pixbuf_calloc(&pbuf, &err);
    if (r)
        abort();

    xoff = (pbuf.pwidth - b.ibounds.width) >> 1;
    yoff = (pbuf.pheight - b.ibounds.height) >> 1;
    xscale = 1.0f / pbuf.pwidth;
    yscale = 1.0f / pbuf.pheight;
    lp->tx0 = xscale * xoff;
    lp->tx1 = xscale * (xoff + b.ibounds.width);
    lp->ty0 = yscale * (pbuf.pheight - yoff);
    lp->ty1 = yscale * (pbuf.pheight - yoff - b.ibounds.height);

    sg_layout_render(lp, &pbuf, xoff - b.ibounds.x, yoff - b.ibounds.y);

    if (!lp->texnum)
        glGenTextures(1, &lp->texnum);
    glPushAttrib(GL_ENABLE_BIT);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, lp->texnum);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0,
                 GL_ALPHA, pbuf.pwidth, pbuf.pheight, 0,
                 GL_ALPHA, GL_UNSIGNED_BYTE, pbuf.data);
    glPopAttrib();
}

void
sg_layout_draw(struct sg_layout *lp)
{
    if (!lp->texnum)
        sg_layout_load(lp);

    if (0) {
        glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
        glDisable(GL_TEXTURE_2D);
        glColor4ub(0, 0, 255, 128);
        glBegin(GL_TRIANGLE_STRIP);
        glVertex2f(lp->vx0, lp->vy0);
        glVertex2f(lp->vx1, lp->vy0);
        glVertex2f(lp->vx0, lp->vy1);
        glVertex2f(lp->vx1, lp->vy1);
        glEnd();
        glPopAttrib();
    }

    glPushAttrib(GL_ENABLE_BIT);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, lp->texnum);
    glBegin(GL_TRIANGLE_STRIP);
    glTexCoord2f(lp->tx0, lp->ty0); glVertex2f(lp->vx0, lp->vy0);
    glTexCoord2f(lp->tx1, lp->ty0); glVertex2f(lp->vx1, lp->vy0);
    glTexCoord2f(lp->tx0, lp->ty1); glVertex2f(lp->vx0, lp->vy1);
    glTexCoord2f(lp->tx1, lp->ty1); glVertex2f(lp->vx1, lp->vy1);
    glEnd();
    glPopAttrib();
}

#if defined(HAVE_CORETEXT)

static void
sg_layoutct_copyrect(struct sg_layout_rect *dest, CGRect *src)
{
    int x1, x2, y1, y2;
    x1 = floorf(CGRectGetMinX(*src));
    y1 = floorf(CGRectGetMinY(*src));
    x2 = ceilf(CGRectGetMaxX(*src));
    y2 = ceilf(CGRectGetMaxY(*src));
    dest->x = x1;
    dest->y = y1;
    dest->width = x2 - x1;
    dest->height = y2 - y1;
}

static CGContextRef
sg_layoutct_dummycontext(void)
{
    static CGContextRef dummy_context;
    static unsigned char dummy_context_data;
    if (!dummy_context) {
        CGColorSpaceRef color_space =
            CGColorSpaceCreateWithName(kCGColorSpaceGenericGray);
        dummy_context = CGBitmapContextCreate(
            &dummy_context_data, 1, 1, 8, 1, color_space, 0);
        CGColorSpaceRelease(color_space);
        if (!dummy_context)
            abort();
    }
    return dummy_context;
}

static void
sg_layoutct_calcbounds(struct sg_layout *lp, struct sg_layout_bounds *b)
{
    CFStringRef string = NULL;
    CGColorRef white = NULL;
    CTFontRef font = NULL;
    CFStringRef keys[2];
    CFTypeRef vals[2];
    CFDictionaryRef attr = NULL;
    CFAttributedStringRef attrstring = NULL;
    CTLineRef line = NULL;
    CGRect ibounds;

    string = CFStringCreateWithBytes(
        kCFAllocatorDefault, (const UInt8 *) lp->text, lp->textlen,
        kCFStringEncodingUTF8, false);
    white = CGColorCreateGenericGray(1.0f, 1.0f);
    font = CTFontCreateWithName(CFSTR("Helvetica"), 16.0f, NULL);

    keys[0] = kCTFontAttributeName;
    vals[0] = font;
    keys[1] = kCTForegroundColorAttributeName;
    vals[1] = white;

    attr = CFDictionaryCreate(
        kCFAllocatorDefault, (const void **) keys, (const void **) vals, 2,
        &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    attrstring = CFAttributedStringCreate(kCFAllocatorDefault, string, attr);
    line = CTLineCreateWithAttributedString(attrstring);
    ibounds = CTLineGetImageBounds(line, sg_layoutct_dummycontext());
    b->x = 0;
    b->y = 0;
    sg_layoutct_copyrect(&b->ibounds, &ibounds);

    CFRelease(attrstring);
    CFRelease(attr);
    CFRelease(font);
    CFRelease(white);
    CFRelease(string);

    lp->ct_line = line;
}

static void
sg_layoutct_render(struct sg_layout *lp, struct sg_pixbuf *pbuf,
                   int xoff, int yoff)
{
    CGColorSpaceRef color_space = NULL;
    CGContextRef context = NULL;
    CTLineRef line = lp->ct_line;

    color_space = CGColorSpaceCreateWithName(kCGColorSpaceGenericGray);
    context = CGBitmapContextCreate(
        pbuf->data, pbuf->pwidth, pbuf->pheight, 8, pbuf->rowbytes,
        color_space, 0);
    CGContextSetTextPosition(context, xoff, yoff);
    CTLineDraw(line, context);

    CFRelease(color_space);
    CFRelease(context);
    CFRelease(line);

    lp->ct_line = NULL;
}

#elif defined(HAVE_PANGOCAIRO)

static void
sg_layoutpc_copyrect(struct sg_layout_rect *dest, PangoRectangle *src)
{
    dest->x = src->x;
    dest->y = -src->y - src->height;
    dest->width = src->width;
    dest->height = src->height;
}

/* Pass NULL to get a context for a dummy Cairo image backend.  */
static PangoContext *
sg_layoutpc_sharedcontext(cairo_t *cr)
{
    /* Just as with the Core Text API on Mac OS X, we need a dummy
       context to bind to in order to calculate bounds.  */
    static cairo_t *dummy_cr;
    static PangoContext *shared_pc;
    cairo_surface_t *dummy_surf;
    if (!cr) {
        if (!dummy_cr) {
            dummy_surf = cairo_image_surface_create(CAIRO_FORMAT_A8, 1, 1);
            if (!dummy_surf)
                abort();
            dummy_cr = cairo_create(dummy_surf);
            if (!dummy_cr)
                abort();
        }
        cr = dummy_cr;
    }
    if (!shared_pc) {
        shared_pc = pango_cairo_create_context(cr);
        if (!shared_pc)
            abort();
    } else {
        pango_cairo_update_context(cr, shared_pc);
    }
    return shared_pc;
}

static void
sg_layoutpc_calcbounds(struct sg_layout *lp, struct sg_layout_bounds *b)
{
    PangoContext *pc;
    PangoLayout *pl;
    PangoFontDescription *pf;
    PangoRectangle ibounds, lbounds;

    pc = sg_layoutpc_sharedcontext(NULL);
    pl = lp->pango_layout;
    if (!pl) {
        pl = pango_layout_new(pc);
        if (!pl)
            abort();
        lp->pango_layout = pl;
    }

    pf = pango_font_description_new();
    if (!pf)
        abort();
    pango_font_description_set_family(pf, "Serif");
    pango_font_description_set_absolute_size(pf, 16 * PANGO_SCALE);
    pango_layout_set_font_description(pl, pf);
    pango_layout_set_alignment(pl, PANGO_ALIGN_LEFT);
    pango_layout_set_text(pl, lp->text, lp->textlen);
    b->x = 0;
    b->y = -(pango_layout_get_baseline(pl) / PANGO_SCALE);
    pango_layout_get_pixel_extents(pl, &ibounds, &lbounds);

    sg_layoutpc_copyrect(&b->ibounds, &ibounds);
    sg_layoutpc_copyrect(&b->lbounds, &lbounds);
}

static void
sg_layoutpc_render(struct sg_layout *lp, struct sg_pixbuf *pbuf,
                   int xoff, int yoff)
{
    cairo_surface_t *surf;
    cairo_t *cr;
    PangoContext *pc;
    PangoLayout *pl;

    surf = cairo_image_surface_create_for_data(
        pbuf->data, CAIRO_FORMAT_A8,
        pbuf->pwidth, pbuf->pheight, pbuf->rowbytes);
    if (!surf)
        abort();
    cr = cairo_create(surf);
    if (!cr)
        abort();
    cairo_translate(cr, xoff, pbuf->pheight - yoff);

    pc = sg_layoutpc_sharedcontext(cr);
    pl = lp->pango_layout;
    if (!pl)
        abort();
    pango_layout_context_changed(pl);
    pango_cairo_show_layout(cr, pl);
    g_object_unref(pl);
    lp->pango_layout = NULL;

    cairo_destroy(cr);
    cairo_surface_destroy(surf);
}

#endif
