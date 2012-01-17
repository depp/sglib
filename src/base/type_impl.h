#ifndef BASE_TYPE_IMPL_H
#define BASE_TYPE_IMPL_H
#ifdef __cplusplus
extern "C" {
#endif
struct sg_pixbuf;

struct sg_layout {
    unsigned refcount;
    char *text;
    unsigned textlen;

    unsigned texnum;

    float vx0, vx1, vy0, vy1;
    float tx0, tx1, ty0, ty1;

    struct sg_layout_impl *impl;
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

/* Free the 'impl' field from an sg_layout structure.  */
void
sg_layout_impl_free(struct sg_layout_impl *li);

/* Calculate the bounds of the given layout.  */
void
sg_layout_calcbounds(struct sg_layout *lp, struct sg_layout_bounds *b);

/* Render the layout at the given location in a pixel buffer.  */
void
sg_layout_render(struct sg_layout *lp, struct sg_pixbuf *pbuf,
                 int xoff, int yoff);

#ifdef __cplusplus
}
#endif
#endif
