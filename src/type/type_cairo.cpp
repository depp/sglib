#include "type.hpp"
#include <cairo/cairo.h>
#include <pango/pangocairo.h>
#include <assert.h>
#include <stdlib.h>

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

#include <stdio.h>
static PangoFontDescription *getFontDescription()
{
    static PangoFontDescription *desc = NULL;
    if (!desc) {
        PangoContext *context = getSharedContext();
        PangoFontMap *fonts = pango_context_get_font_map(context);
        PangoFontFamily **families;
        int nfamilies;
        pango_font_map_list_families(fonts, &families, &nfamilies);
        printf("Fonts:\n");
        for (int i = 0; i < nfamilies; ++i)
            printf("  %s\n", pango_font_family_get_name(families[i]));
        g_free(families);
        desc = pango_font_description_new();
        pango_font_description_set_family_static(desc, "URW Chancery L");
        pango_font_description_set_size(desc, 24 * PANGO_SCALE);
    }
    return desc;
}

void Type::loadImage(void **data, unsigned int *width,
                     unsigned int *height)
{
    PangoContext *context = getSharedContext();
    PangoLayout *layout = pango_layout_new(context);
    PangoFontDescription *desc = getFontDescription();
    pango_layout_set_font_description(layout, desc);
    pango_layout_set_text(layout, text_.data(), text_.size());
    int baseline = pango_layout_get_baseline(layout) / PANGO_SCALE;
    PangoRectangle bounds;
    pango_layout_get_pixel_extents(layout, &bounds, NULL);
    vx1_ = bounds.x;
    vx2_ = bounds.x + bounds.width;
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
