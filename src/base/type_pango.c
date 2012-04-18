#include "pixbuf.h"
#include "type_impl.h"
#include "version.h"
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

void
sg_layout_impl_free(struct sg_layout_impl *li)
{
    if (li->layout)
        g_object_unref(li->layout);
    free(li);
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

void
sg_layout_calcbounds(struct sg_layout *lp, struct sg_layout_bounds *b)
{
    struct sg_layout_impl *li;
    PangoContext *pc;
    PangoLayout *pl;
    PangoFontDescription *pf;
    PangoRectangle ibounds, lbounds;

    pc = sg_layout_sharedcontext(NULL);
    li = lp->impl;
    if (!li) {
        li = malloc(sizeof(*li));
        if (!li)
            abort();
        li->layout = NULL;
        lp->impl = li;
    }
    pl = li->layout;
    if (!pl) {
        pl = pango_layout_new(pc);
        if (!pl)
            abort();
        li->layout = pl;
    }

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
    b->x = 0;
    b->y = -(pango_layout_get_baseline(pl) / PANGO_SCALE);
    pango_layout_get_pixel_extents(pl, &ibounds, &lbounds);

    sg_layout_copyrect(&b->ibounds, &ibounds);
    sg_layout_copyrect(&b->lbounds, &lbounds);
}

/* FIXME: assums li is not NULL? */
void
sg_layout_render(struct sg_layout *lp, struct sg_pixbuf *pbuf,
                 int xoff, int yoff)
{
    cairo_surface_t *surf;
    cairo_t *cr;
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

    (void) sg_layout_sharedcontext(cr);
    pl = lp->impl->layout;
    if (!pl)
        abort();
    pango_layout_context_changed(pl);
    pango_cairo_show_layout(cr, pl);
    g_object_unref(pl);
    lp->impl->layout = NULL;

    cairo_destroy(cr);
    cairo_surface_destroy(surf);
}

void
sg_version_pango(struct sg_logger *lp)
{
    sg_version_lib(lp, "Pango", PANGO_VERSION_STRING,
                   pango_version_string());
}
