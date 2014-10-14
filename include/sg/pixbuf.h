/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#ifndef SG_PIXBUF_H
#define SG_PIXBUF_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
struct sg_error;
struct sg_file;

/**
 * @brief A list of supported image extensions, without the leading
 * `'.'`, and separated by `':'`.
 */
extern const char SG_PIXBUF_IMAGE_EXTENSIONS[];

/**
 * @brief Pixel formats.
 */
typedef enum {
    /* 8-bit formats.  Y means grayscale.  All formats with alpha are
       assumed to use premultiplied alpha unless otherwise
       specified.  */
    /** @brief 8-bit grayscale.  */
    SG_Y,
    /** @brief 8-bit grayscale with premultiplied alpha.  */
    SG_YA,
    /** @brief 8-bit RGB, packed.  */
    SG_RGB,
    /** @brief 8-bit RGB, with an unused byte.  */
    SG_RGBX,
    /** @brief 8-bit RGB with premultiplied alpha.  */
    SG_RGBA
} sg_pixbuf_format_t;

/**
 * @brief The number of pixel formats.
 */
#define SG_PIXBUF_NFORMAT ((int) SG_RGBA + 1)

/**
 * @brief Map from pixel formats to pixel size, in bytes.
 */
extern const size_t SG_PIXBUF_FORMATSIZE[SG_PIXBUF_NFORMAT];

/**
 * @brief Map from pixel formats to their names.
 */
extern const char SG_PIXBUF_FORMATNAME[SG_PIXBUF_NFORMAT][5];

/**
 * @brief Pixel buffer.
 *
 * The buffer consists of an image within a larger buffer.
 */
struct sg_pixbuf {
    /** @brief Pointer to pixel data.  */
    void *data;
    /** @brief The pixel format.  */
    sg_pixbuf_format_t format;
    /** @brief Image width.  */
    int iwidth;
    /** @brief Image height.  */
    int iheight;
    /** @brief Pixel buffer width.  */
    int pwidth;
    /** @brief Pixel buffer height.  */
    int pheight;
    /** @brief Number of bytes per row of pixels.  */
    int rowbytes;
};

/**
 * @brief Initialize a pixel buffer.
 *
 * @param pbuf The pixel buffer.
 */
void
sg_pixbuf_init(struct sg_pixbuf *pbuf);

/**
 * @brief Destroy a pixel buffer, freeing the pixel data with `free()`.
 *
 * @param pbuf The pixel buffer.
 */
void
sg_pixbuf_destroy(struct sg_pixbuf *pbuf);

/**
 * @brief Set the dimensions and format of the pixel buffer.
 *
 * Only the image size can be specified, the pixel buffer size will be
 * rounded up so the dimensions are powers of two.
 *
 * This will fail if the dimensions or format are not supported.
 *
 * @brief pbuf The pixel buffer.
 * @brief format The pixel format.
 * @brief width The image width.
 * @brief height The image height.
 * @brief err On failure, the error.
 * @return Zero for success, nonzero for failure.
 */
int
sg_pixbuf_set(struct sg_pixbuf *pbuf, sg_pixbuf_format_t format,
              int width, int height, struct sg_error **err);

/**
 * @brief Allocate memory for a pixel buffer.
 *
 * @param pbuf The pixel buffer.
 * @param err On failure, the error.
 * @return Zero for success, nonzero for failure.
 */
int
sg_pixbuf_alloc(struct sg_pixbuf *pbuf, struct sg_error **err);

/**
 * @brief Allocate zeroed memory for a pixel buffer.
 *
 * @param pbuf The pixel buffer.
 * @param err On failure, the error.
 * @return Zero for success, nonzero for failure.
 */
int
sg_pixbuf_calloc(struct sg_pixbuf *pbuf, struct sg_error **err);

/**
 * @brief Load an image from a memory buffer.
 *
 * The memory buffer format (PNG, JPEG, etc.) will be automatically
 * detected.
 *
 * @param pbuf The pixel buffer.
 * @param data Pointer to the memory buffer.
 * @param len Length of the memory buffer.
 * @param err On failure, the error.
 * @return Zero for success, nonzero for failure.
 */
int
sg_pixbuf_loadimage(struct sg_pixbuf *pbuf, const void *data, size_t len,
                    struct sg_error **err);

/**
 * @brief Write an image to a file using the PNG format.
 *
 * @param pbuf The pixel buffer to write.
 * @param fp The file to write the image to.
 * @param err On failure, the error.
 * @return Zero for success, nonzero for failure.
 */
int
sg_pixbuf_writepng(struct sg_pixbuf *pbuf, struct sg_file *fp,
                   struct sg_error **err);

/**
 * @brief Convert image data from non-premultiplied alpha to
 * premultiplied alpha.
 *
 * @param pbuf The pixel buffer.
 */
void
sg_pixbuf_premultiply_alpha(struct sg_pixbuf *pbuf);

#ifdef __cplusplus
}
#endif
#endif
