/* Copyright 2012-2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#ifndef SG_TYPE_H
#define SG_TYPE_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
struct sg_error;

/**
 * @file sg/type.h
 *
 * @brief Type and text rendering.
 */

/**
 * @brief Text alignment constants.
 */
typedef enum {
    /** @brief Align the text on the left margin.  */
    SG_TEXTALIGN_LEFT,
    /** @brief Align the text on the right margin.  */
    SG_TEXTALIGN_RIGHT,
    /** @brief Center the text.  */
    SG_TEXTALIGN_CENTER
} sg_textalign_t;

/**
 * @brief A point in the text layout system
 */
struct sg_textpoint {
    /** @brief X coordinate */
    short x;
    /** @brief Y coordinate */
    short y;
};

/**
 * @brief A rectangle in the text layout system
 */
struct sg_textrect {
    /** @brief Minimum x coordinate */
    short x0;
    /** @brief Minimum y coordinate */
    short y0;
    /** @brief Maximum x coordinate */
    short x1;
    /** @brief Maximum y coordinate */
    short y1;
};

/**
 * @brief Metrics for a text layout
 */
struct sg_textmetrics {
    /**
     * @brief Logical bounds.
     *
     * May be smaller or larger than the pixel bounds.
     */
    struct sg_textrect logical;

    /**
     * @brief Pixel bounds.
     *
     * May be smaller or larger than the logical bounds.
     */
    struct sg_textrect pixel;

    /**
     * @brief The Y coordinate of the baseline of the first line.
     */
    int baseline;
};

/**********************************************************************/

/**
 * @brief A typeface.
 */
struct sg_typeface;

/**
 * @brief Increment a typeface's reference count.
 */
void
sg_typeface_incref(struct sg_typeface *tp);

/**
 * @brief Decrement a typeface's reference count.
 */
void
sg_typeface_decref(struct sg_typeface *tp);

/**
 * @brief Load a typeface from a file.
 *
 * This looks for TTF, OTF, and WOFF files.
 *
 * @param path Path to the file, without extension.
 * @param pathlen Length of the path.
 * @param err On failure, the error.
 * @return A reference to a typeface, or `NULL` for failure.
 */
struct sg_typeface *
sg_typeface_file(const char *path, size_t pathlen, struct sg_error **err);

/**********************************************************************/

/**
 * @brief Bitmap font
 */
struct sg_font;

/**
 * @brief Increment a bitmap font's reference count.
 */
void
sg_font_incref(struct sg_font *fp);

/**
 * @brief Decrement a bitmap font's reference count.
 */
void
sg_font_decref(struct sg_font *fp);

/**
 * @brief Create a font from a typeface.
 *
 * This may return a reference to an existing font.
 *
 * @param tp The typeface.
 * @param size The font size.
 * @param err On failure, the error.
 * @return A reference to bitmap font, or `NULL` on failure.
 */
struct sg_font *
sg_font_new(struct sg_typeface *tp, float size, struct sg_error **err);

/**********************************************************************/

/**
 * @brief Text flow
 *
 * A text flow consists of the text, styles, and all data necessary to
 * create a text layout.
 */
struct sg_textflow;

/**
 * @brief Create a new text flow object.
 */
struct sg_textflow *
sg_textflow_new(struct sg_error **err);

/**
 * @brief Delete a text flow.
 */
void
sg_textflow_free(struct sg_textflow *flow);

/**
 * @brief Add text to a text flow.
 *
 * @param flow The text flow to modify.
 * @param text The text to add, encoded in UTF-8.
 * @param length The text length, in bytes.
 */
void
sg_textflow_addtext(struct sg_textflow *flow,
                    const char *text, size_t length);

/**
 * @brief Set the font for text added to a text flow.
 *
 * This does not affect text already in the flow.
 *
 * @param flow The text flow to modify.
 * @param font The font to use.
 */
void
sg_textflow_setfont(struct sg_textflow *flow, struct sg_font *font);

/**
 * @brief Set the maximum width for text in a text flow.
 *
 * Lines that extend beyond this width are wrapped or truncated.  A
 * negative or zero width indicates unlimited width, which is the
 * default.  Note that the actual pixel bounds may extend beyond this.
 *
 * @param flow The text flow to modify.
 * @param width The new width.
 */
void
sg_textflow_setwidth(struct sg_textflow *flow, float width);

/**********************************************************************/

/**
 * @brief Text layout
 *
 * A text layout consists of the graphics data necessary to draw text
 * to the screen.
 */
struct sg_textlayout;

/**
 * @brief Create a text layout object.
 *
 * The OpenGL context must be active.
 *
 * @param flow The text flow to use for layout.
 * @param err On failure, the error.
 * @return The text layout, or `NULL` if an error occurred.
 */
struct sg_textlayout *
sg_textlayout_new(struct sg_textflow *flow, struct sg_error **err);

/**
 * @brief Free a text layout object.
 *
 * The OpenGL context must be active.
 *
 * @param layout The layout to free.
 */
void
sg_textlayout_free(struct sg_textlayout *layout);

/**
 * @brief Get the metrics for a text layout.
 *
 * @brief layout The text layout to query.
 * @brief metrics On return, the text metrics.
 */
void
sg_textlayout_getmetrics(struct sg_textlayout *layout,
                         struct sg_textmetrics *metrics);

/**
 * @brief Draw a text layout to the screen.
 *
 * The program must take a vec4 attribute input at index 0, where the
 * first two components are vertex coordinates and the last two
 * components are texture coordinates.  The texture coordinates are
 * scaled by a vec2 uniform, whose location is given by the
 * texture_scale parameter.  The active texture unit will be used to
 * provide textures for the program.
 *
 * @param layout The text layout to draw.
 * @param array The vertex array attribute.
 * @param texture_scale The `vec2` uniform for the texture scale.
 */
void
sg_textlayout_draw(struct sg_textlayout *layout,
                   unsigned attrib, unsigned texture_scale);

#ifdef __cplusplus
}
#endif
#endif
