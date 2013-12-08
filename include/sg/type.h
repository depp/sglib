/* Copyright 2012-2013 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#ifndef SG_TYPE_H
#define SG_TYPE_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
struct sg_error;
struct sg_pixbuf;

/**
 * @file sg/type.h
 *
 * @brief Type and text rendering.
 */

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
struct sg_textlayout_metrics {
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
     * @brief The origin.
     *
     * For text with left alignment, this is the left edge of the
     * first baseline.  For right alignment, this is the right edge of
     * the first baseline.  For center alignment, this is the center
     * of the first baseline.
     */
    struct sg_textpoint origin;
};

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

struct sg_textbitmap;

/**
 * @brief Create a simple text bitmap using the platform's text layout
 * engine.
 *
 * Note that the results are platform dependent.  This will use Core
 * Text on OS X, Pango on Linux, Uniscribe on Windows, etc.  The
 * available fonts depend on the system and no fonts are guaranteed to
 * be available.
 *
 * Text alignment may still affect the results even if the text spans multiple lines.
 *
 * @param text The text, encoded in UTF-8.
 * @param textlen The length of the text in bytes.
 * @param fontname The name of the font to use, NUL-terminated.
 * @param fontsize The size of the font to use, in pixels.
 * @param align The horizontal text alignment.
 * @param width The width at which to wrap the text, or a negative
 * number to indicate that the text should not wrap.
 * @param err On failure, the error.
 *
 * @return A new text bitmap
 */
struct sg_textbitmap *
sg_textbitmap_new_simple(const char *text, size_t textlen,
                         const char *fontname, double fontsize,
                         sg_textalign_t alignment, double width,
                         struct sg_error **err);

/**
 * @brief Free a text bitmap object.
 *
 * @param bitmap The bitmap
 */
void
sg_textbitmap_free(struct sg_textbitmap *bitmap);

/**
 * @brief Get the metrics for a text bitmap.
 *
 * @param bitmap The text bitmap.
 * @param metrics On return, the layout metrics.
 * @param err On failure, the error.
 * @return Zero for success or nonzero for failure.
 */
int
sg_textbitmap_getmetrics(struct sg_textbitmap *bitmap,
                         struct sg_textlayout_metrics *metrics,
                         struct sg_error **err);

/**
 * @brief Render a text bitmap into a pixel buffer.
 *
 * Note that this uses the convention that the origin is at the bottom
 * left of the pixel buffer, and positive Y coordinates are up.
 *
 * @param bitmap The source text bitmap
 * @param pixbuf The target pixel buffer, already initialized
 * @param offset The offset at which to draw the text
 * @param err On failure, the error
 * @return Zero on success, nonzero on failure
 */
int
sg_textbitmap_render(struct sg_textbitmap *bitmap,
                     struct sg_pixbuf *pixbuf,
                     struct sg_textpoint offset,
                     struct sg_error **err);

#ifdef __cplusplus
}
#endif
#endif
