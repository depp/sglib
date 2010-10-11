#include "rastertext.hpp"
#include "font_pango.hpp"
#include <cairo/cairo.h>
#include <pango/pangocairo.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

static unsigned int round_up_pow2(unsigned int x)
{
    x -= 1;
    x |= x >> 16;
    x |= x >> 8;
    x |= x >> 4;
    x |= x >> 2;
    x |= x >> 1;
    return x + 1;
}

static PangoContext *getSharedContext()
{
    static cairo_t *cr = NULL;
    static PangoContext *context = NULL;
    if (!cr) {
        cairo_surface_t *surf =
            cairo_image_surface_create(CAIRO_FORMAT_A8, 1, 1);
        assert(surf);
        cr = cairo_create(surf);
    }
    if (!context) {
        context = pango_cairo_create_context(cr);
        assert(context);
    } else {
        pango_cairo_update_context(cr, context);
    }
    return context;
}

void RasterText::loadImage(void **data, unsigned int *width,
                           unsigned int *height)
{
    PangoContext *context = getSharedContext();
    PangoLayout *layout = pango_layout_new(context);
    if (font_.info_) {
        PangoFontDescription *desc = font_.info_->desc;
        if (desc)
            pango_layout_set_font_description(layout, desc);
    }
    PangoAlignment alignment = PANGO_ALIGN_LEFT;
    pango_layout_set_alignment(layout, alignment);
    pango_layout_set_text(layout, text_.data(), text_.size());
    int baseline = pango_layout_get_baseline(layout) / PANGO_SCALE;
    PangoRectangle bounds, lbounds;
    pango_layout_get_pixel_extents(layout, &bounds, &lbounds);
    float lxoff;
    switch (alignment_) {
    default:
    case Left:
        lxoff = bounds.x - lbounds.x;
        break;
    case Center:
        lxoff = bounds.x - (lbounds.x + lbounds.width / 2);
        break;
    case Right:
        lxoff = bounds.x - (lbounds.x + lbounds.width);
        break;
    }
    vx1_ = bounds.x + lxoff;
    vx2_ = bounds.x + bounds.width + lxoff;
    vy1_ = baseline - bounds.y - bounds.height;
    vy2_ = baseline - bounds.y;
    unsigned int twidth = round_up_pow2(bounds.width);
    unsigned int theight = round_up_pow2(bounds.height);
    float xscale = 1.0f / twidth, yscale = 1.0f / theight;
    int xoff = (twidth - bounds.width) / 2;
    int yoff = (theight - bounds.height) / 2;
    tx1_ = xoff * xscale;
    tx2_ = (xoff + bounds.width) * xscale;
    ty1_ = (yoff + bounds.height) * yscale;
    ty2_ = yoff * yscale;
    unsigned int stride =
        cairo_format_stride_for_width(CAIRO_FORMAT_A8, twidth);
    void *ptr = calloc(stride, theight);
    if (!ptr)
        return;
    cairo_surface_t *surf = cairo_image_surface_create_for_data(
        static_cast<unsigned char *>(ptr), CAIRO_FORMAT_A8,
        twidth, theight, stride);
    cairo_t *cr = cairo_create(surf);
    cairo_translate(cr, xoff - bounds.x, yoff - bounds.y);
    pango_cairo_update_context(cr, context);
    pango_layout_context_changed(layout);
    pango_cairo_show_layout(cr, layout);
    g_object_unref(layout);
    cairo_destroy(cr);
    cairo_surface_destroy(surf);
    *data = ptr;
    *width = twidth;
    *height = theight;
}
