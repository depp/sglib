#ifndef BASE_TYPE_IMPL_H
#define BASE_TYPE_IMPL_H
#ifdef __cplusplus
extern "C" {
#endif
struct sg_pixbuf;

struct sg_layout {
    unsigned refcount;

    /* Layout text contents */
    char *text;
    unsigned textlen;

    /* OpenGL texture */
    unsigned texnum;

    /* Texture and vertex coordinates */
    float vx0, vx1, vy0, vy1;
    float tx0, tx1, ty0, ty1;

    /* Layout width, or -1 for unlimited */
    float width;

    /* Style */
    char *family;
    float size;
};

struct sg_style {
    unsigned refcount;
    char *family;
    float size;
};

/* The rectangle uses OpenGL-style coordinates (origin at bottom
   left), even though the pixel data will be ordered normally (top
   left at buffer offset zero).  */
struct sg_layout_rect {
    int x, y, width, height;
};

struct sg_layout_bounds {
    /* The "origin" of the text when it is drawn at (0, 0).  The
       origin of the text is the logical left edge of the
       baseline.  */
    int x, y;

    /* The ink bounds of the text.  All pixels are contained in this
       rectangle.  */
    struct sg_layout_rect ibounds;

    /* The logical bounds of the text.  May be smaller or larger than
       the ink bounds.  */
    struct sg_layout_rect lbounds;
};

/* Backend-specific layout implementation state.  Created temporarily
   to draw the layout.  */
struct sg_layout_impl;

/* Create a layout implementation object from the given layout.  */
struct sg_layout_impl *
sg_layout_impl_new(struct sg_layout *lp);

void
sg_layout_impl_free(struct sg_layout_impl *li);

/* Calculate the bounds of the given layout.  */
void
sg_layout_impl_calcbounds(struct sg_layout_impl *li,
                          struct sg_layout_bounds *b);

/* Render the layout at the given location in a pixel buffer.  */
void
sg_layout_impl_render(struct sg_layout_impl *li, struct sg_pixbuf *pbuf,
                      int xoff, int yoff);

#ifdef __cplusplus
}
#endif
#endif
