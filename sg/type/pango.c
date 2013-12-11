/* Copyright 2012-2013 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "sg/error.h"
#include "sg/pixbuf.h"
#include "sg/type.h"
#include "sg/version.h"
#include <math.h>
#include <pango/pango.h>
#include <pango/pangocairo.h>
#include <stdlib.h>

struct sg_textbitmap {
    PangoLayout *layout;
};

/* Convert a floating point number to integer Pango units.  */
static int
pango_size(double x)
{
    return floor(x * PANGO_SCALE + 0.5);
}

static void
sg_textbitmap_copyrect(struct sg_textrect *dest, PangoRectangle *src)
{
    dest->x0 = src->x;
    dest->y0 = -src->y - src->height;
    dest->x1 = src->x + src->width;
    dest->y1 = -src->y;
}

/* Pass NULL to get a context for a dummy Cairo image backend.  */
static PangoContext *
sg_textbitmap_sharedcontext(cairo_t *cr)
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

struct sg_textbitmap *
sg_textbitmap_new_simple(const char *text, size_t textlen,
                         const char *fontname, double fontsize,
                         sg_textalign_t alignment, double width,
                         struct sg_error **err)
{
    struct sg_textbitmap *bitmap = NULL;
    PangoContext *pc;
    PangoLayout *pl = NULL;
    PangoFontDescription *pf = NULL;
    PangoAlignment palign;

    bitmap = malloc(sizeof(*bitmap));
    if (!bitmap)
        goto nomem;
    bitmap->layout = NULL;

    pc = sg_textbitmap_sharedcontext(NULL);

    pl = pango_layout_new(pc);
    if (!pl)
        goto nomem;
    bitmap->layout = pl;

    pf = pango_font_description_new();
    if (!pf)
        goto nomem;
    pango_font_description_set_family(pf, fontname);
    pango_font_description_set_absolute_size(pf, pango_size(fontsize));
    pango_layout_set_font_description(pl, pf);
    pango_font_description_free(pf);
    pf = NULL;

    switch (alignment) {
    default:
    case SG_TEXTALIGN_LEFT: palign = PANGO_ALIGN_LEFT; break;
    case SG_TEXTALIGN_RIGHT: palign = PANGO_ALIGN_RIGHT; break;
    case SG_TEXTALIGN_CENTER: palign = PANGO_ALIGN_CENTER; break;
    }
    pango_layout_set_alignment(pl, palign);

    if (width > 0.0)
        pango_layout_set_width(pl, pango_size(width));

    pango_layout_set_text(pl, text, textlen);
    return bitmap;

nomem:
    sg_error_nomem(err);
    free(bitmap);
    if (pl)
        g_object_unref(pl);
    if (pf)
        pango_font_description_free(pf);
    return NULL;
}

void
sg_textbitmap_free(struct sg_textbitmap *bitmap)
{
    if (bitmap->layout)
        g_object_unref(bitmap->layout);
    free(bitmap);
}

int
sg_textbitmap_getmetrics(struct sg_textbitmap *bitmap,
                         struct sg_textlayout_metrics *metrics,
                         struct sg_error **err)
{
    PangoLayout *pl = bitmap->layout;
    PangoRectangle ibounds, lbounds;

    (void) err;

    metrics->baseline = -(pango_layout_get_baseline(pl) / PANGO_SCALE);
    pango_layout_get_pixel_extents(pl, &ibounds, &lbounds);

    sg_textbitmap_copyrect(&metrics->logical, &lbounds);
    sg_textbitmap_copyrect(&metrics->pixel, &ibounds);

    return 0;
}

int
sg_textbitmap_render(struct sg_textbitmap *bitmap,
                     struct sg_pixbuf *pixbuf,
                     struct sg_textpoint offset,
                     struct sg_error **err)
{
    cairo_surface_t *surf;
    cairo_t *cr;
    PangoLayout *pl = bitmap->layout;

    (void) err;

    surf = cairo_image_surface_create_for_data(
        pixbuf->data, CAIRO_FORMAT_A8,
        pixbuf->pwidth, pixbuf->pheight, pixbuf->rowbytes);
    if (!surf)
        abort();
    cr = cairo_create(surf);
    if (!cr)
        abort();
    cairo_translate(cr, offset.x, pixbuf->pheight - offset.y);

    (void) sg_textbitmap_sharedcontext(cr);
    pango_layout_context_changed(pl);
    pango_cairo_show_layout(cr, pl);

    cairo_destroy(cr);
    cairo_surface_destroy(surf);

    return 0;
}

void
sg_version_pango(struct sg_logger *lp)
{
    sg_version_lib(lp, "Pango", PANGO_VERSION_STRING,
                   pango_version_string());
}
