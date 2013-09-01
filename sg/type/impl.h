/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
struct sg_pixbuf;

/* The data structures use OpenGL-style coordinates (origin at bottom
   left), even though the pixel data will be ordered normally (top
   left at buffer offset zero).  */

struct sg_layout_point {
    int x, y;
};

struct sg_layout_rect {
    int x, y, width, height;
};

struct sg_layout_bounds {
    /* The "origin" of the text when it is drawn at (0, 0).  The
       origin of the text is the logical left edge of the baseline
       (for left-to right text, anyway).  */
    int x, y;

    /* The pixel bounds of the text.  All pixels are contained in this
       rectangle.  */
    struct sg_layout_rect pixel;

    /* The logical bounds of the text.  May be smaller or larger than
       the ink bounds.  */
    struct sg_layout_rect logical;
};

struct sg_layout {
    unsigned refcount;

    /* Layout text contents */
    char *text;
    unsigned textlen;

    /* OpenGL texture */
    unsigned texnum;

    /* Texture and coordinates of ink bounds */
    float tx0, tx1, ty0, ty1;

    struct sg_layout_bounds bounds;

    /* Layout width, or -1 for unlimited */
    float width;

    /* Box alignment.  This is managed by the common code, the
       platform code only needs to provide bounding boxes.  */
    int boxalign;

    /* Style */
    char *family;
    float size;
};

struct sg_style {
    unsigned refcount;
    char *family;
    float size;
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
