/* Copyright 2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "sg/type.h"
#include <ft2build.h>
#include FT_FREETYPE_H

struct sg_error;

struct sg_typeface {
    int refcount;

    /* Path to the typeface file loaded.  */
    const char *path;
    int pathlen;

    /* Buffer containing the typeface file.  */
    struct sg_filedata *data;
    /* The FreeType typeface.  */
    FT_Face face;
    /* Linked list of all fonts derived from this typeface.  */
    struct sg_font *font;
};

struct sg_font {
    int refcount;

    /* Size, in FreeType 26.6 units.  */
    int size;
    /* Coordinates of ascender and descender lines, in pixels.  */
    short ascender, descender;
    /* The typeface this font is derived from.  */
    struct sg_typeface *typeface;
    /* The next font derived from the same typeface.  */
    struct sg_font *next;
    /* Array of information about each bitmap glyph.  */
    struct sg_font_glyph *glyph;
    /* Number of glyphs.  */
    int glyphcount;
    /* OpenGL texture containing glyph bitmaps.  */
    unsigned texture;
    /* Factor for scaling from pixel to texture coordinates.  */
    float texscale[2];
};

struct sg_font_glyph {
    /* Glyph size */
    short w, h;
    /* Glyph location in the texture */
    short x, y;
    /* Bitmap location relative to pen */
    short bx, by;
    /* Pen advance */
    short advance;
};

/* A text flow is a sequence of glyphs, but without locations.  */
struct sg_textflow {
    /* The error that occurred when creating this text flow.  */
    struct sg_error *err;
    /* Runs of glyphs using the same font.  */
    struct sg_textflow_run *run;
    unsigned runcount;
    unsigned runalloc;
    /* Glyphs in the text flow.  */
    struct sg_textflow_glyph *glyph;
    unsigned glyphcount;
    unsigned glyphalloc;
    /* Number of glyphs with bitmap data */
    unsigned drawcount;
    /* If positive, the width at which words are wrapped.  */
    int width;
};

struct sg_textflow_run {
    struct sg_font *font;
    unsigned count;
    unsigned drawcount;
};

enum {
    /* The glyph is visible.  */
    SG_TEXTFLOW_VISIBLE = 1u << 0,
    /* The glyph represents a space.  */
    SG_TEXTFLOW_SPACE = 1u << 1
};

struct sg_textflow_glyph {
    unsigned short index;
    unsigned short flags;
};

struct sg_textlayout {
    struct sg_textmetrics metrics;
    unsigned buffer;
    unsigned batchcount;
    struct sg_textlayout_batch *batch;
};

struct sg_textlayout_batch {
    struct sg_font *font;
    int offset;
    int count;
};

extern const struct sg_error_domain SG_ERROR_FREETYPE;

void
sg_error_freetype(struct sg_error **err, int code);
