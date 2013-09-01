/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "impl.h"
#include "sg/pixbuf.h"
#include "sg/version.h"
#include <math.h>
#include <pango/pango.h>
#include <pango/pangocairo.h>
#include <stdlib.h>

struct sg_layout_impl {
    PangoLayout *layout;
};

/* Convert a floating point number to integer Pango units.  */
static int
pango_size(float x)
{
    return floorf(x * PANGO_SCALE + 0.5f);
}

static void
sg_layout_copyrect(struct sg_layout_rect *dest, PangoRectangle *src)
{
    dest->x = src->x;
    dest->y = -src->y - src->height;
    dest->width = src->width;
    dest->height = src->height;
}

/* Pass NULL to get a context for a dummy Cairo image backend.  */
static PangoContext *
sg_layout_sharedcontext(cairo_t *cr)
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

struct sg_layout_impl *
sg_layout_impl_new(struct sg_layout *lp)
{
    struct sg_layout_impl *li;
    PangoContext *pc;
    PangoLayout *pl;
    PangoFontDescription *pf;

    li = malloc(sizeof(*li));
    if (!li)
        abort();
    li->layout = NULL;

    pc = sg_layout_sharedcontext(NULL);

    pl = pango_layout_new(pc);
    if (!pl) abort();
    li->layout = pl;

    pf = pango_font_description_new();
    if (!pf)
        abort();
    pango_font_description_set_family(
        pf, lp->family ? lp->family : "Liberation Sans");
    pango_font_description_set_absolute_size(pf, pango_size(lp->size));
    pango_layout_set_font_description(pl, pf);
    pango_layout_set_alignment(pl, PANGO_ALIGN_LEFT);
    if (lp->width >= 0)
        pango_layout_set_width(pl, pango_size(lp->width));
    pango_layout_set_text(pl, lp->text, lp->textlen);
    return li;
}

void
sg_layout_impl_free(struct sg_layout_impl *li)
{
    if (li->layout)
        g_object_unref(li->layout);
    free(li);
}

void
sg_layout_impl_calcbounds(struct sg_layout_impl *li,
                          struct sg_layout_bounds *b)
{
    PangoLayout *pl = li->layout;
    PangoRectangle ibounds, lbounds;

    b->x = 0;
    b->y = -(pango_layout_get_baseline(pl) / PANGO_SCALE);
    pango_layout_get_pixel_extents(pl, &ibounds, &lbounds);

    sg_layout_copyrect(&b->pixel, &ibounds);
    sg_layout_copyrect(&b->logical, &lbounds);
}

void
sg_layout_impl_render(struct sg_layout_impl *li, struct sg_pixbuf *pbuf,
                      int xoff, int yoff)
{
    cairo_surface_t *surf;
    cairo_t *cr;
    PangoLayout *pl = li->layout;

    surf = cairo_image_surface_create_for_data(
        pbuf->data, CAIRO_FORMAT_A8,
        pbuf->pwidth, pbuf->pheight, pbuf->rowbytes);
    if (!surf)
        abort();
    cr = cairo_create(surf);
    if (!cr)
        abort();
    cairo_translate(cr, xoff, pbuf->pheight - yoff);

    (void) sg_layout_sharedcontext(cr);
    pango_layout_context_changed(pl);
    pango_cairo_show_layout(cr, pl);

    cairo_destroy(cr);
    cairo_surface_destroy(surf);
}

void
sg_version_pango(struct sg_logger *lp)
{
    sg_version_lib(lp, "Pango", PANGO_VERSION_STRING,
                   pango_version_string());
}
