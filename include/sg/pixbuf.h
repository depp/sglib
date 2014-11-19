/* Copyright 2012-2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#ifndef SG_PIXBUF_H
#define SG_PIXBUF_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
struct sg_error;
struct sg_filedata;

/**
 * @brief A list of supported image extensions, without the leading
 * `'.'`, and separated by `':'`.
 */
extern const char SG_PIXBUF_IMAGE_EXTENSIONS[];

/**
 * @brief Pixel formats.
 */
typedef enum {
    /** @brief One 8-bit channel.  */
    SG_R,
    /** @brief Two 8-bit channels.  */
    SG_RG,
    /** @brief Three 8-bit channels, with an unused padding byte.  */
    SG_RGBX,
    /** @brief Four 8-bit channels.  */
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
 * The pixel buffer structure refers to a buffer of memory, but that
 * buffer is not managed by the pixel buffer.  It must be freed and
 * allocated separately.
 */
struct sg_pixbuf {
    /** @brief Pointer to pixel data.  */
    void *data;
    /** @brief The pixel format.  */
    sg_pixbuf_format_t format;
    /** @brief Pixel buffer width.  */
    int width;
    /** @brief Pixel buffer height.  */
    int height;
    /** @brief Number of bytes per row of pixels.  */
    int rowbytes;
};

/**
 * @brief Allocate memory for a pixel buffer.
 *
 * This will assume the buffer is uninitialized.  This will fail if
 * the dimensions or format are not supported.  The buffer data should
 * be later freed with `free()`.
 *
 * @param pbuf The pixel buffer.
 * @brief format The pixel format.
 * @brief width The image width.
 * @brief height The image height.
 * @param err On failure, the error.
 * @return Zero for success, nonzero for failure.
 */
int
sg_pixbuf_alloc(struct sg_pixbuf *pbuf, sg_pixbuf_format_t format,
                int width, int height, struct sg_error **err);

/**
 * @brief Allocate zeroed memory for a pixel buffer.
 *
 * This will assume the buffer is uninitialized.  This will fail if
 * the dimensions or format are not supported.  The buffer data should
 * be later freed with `free()`.
 *
 * @param pbuf The pixel buffer.
 * @brief format The pixel format.
 * @brief width The image width.
 * @brief height The image height.
 * @param err On failure, the error.
 * @return Zero for success, nonzero for failure.
 */
int
sg_pixbuf_calloc(struct sg_pixbuf *pbuf, sg_pixbuf_format_t format,
                 int width, int height, struct sg_error **err);

/**
 * @brief Write an image to a file using the PNG format.
 *
 * @param pbuf The pixel buffer to write.
 * @param path The path to the file to write the image to.
 * @param pathlen The length of the path.
 * @param err On failure, the error.
 * @return Zero for success, nonzero for failure.
 */
int
sg_pixbuf_writepng(struct sg_pixbuf *pbuf, const char *path, size_t pathlen,
                   struct sg_error **err);

/**
 * @brief Upload a pixel buffer as an OpenGL texture.
 *
 * @param pbuf The pixel buffer containing data to upload.
 */
void
sg_pixbuf_texture(struct sg_pixbuf *pbuf);

/**
 * @brief Image flags.
 */
enum {
    /** @brief Indicates that the image is color (not grayscale).  */
    SG_IMAGE_COLOR = 1u << 0,
    /** @brief Indicates that the image has an alpha channel.  */
    SG_IMAGE_ALPHA = 1u << 1
};

/**
 * @brief An image.
 *
 * The image is stored in memory in a form close to its original
 * representation, e.g., compressed.  Do not copy or attempt to
 * allocate this structure, this structure just contains the virtual
 * functions and is part of a larger structure with multiple potential
 * implementations.
 */
struct sg_image {
    /**
     * @brief The image width.
     */
    int width;

    /**
     * @brief The image height.
     */
    int height;

    /**
     * @brief Image flags, indicating the presence of color and alpha.
     */
    unsigned flags;

    /**
     * @brief Free this image.
     *
     * @param img This image.
     */
    void (*free)(struct sg_image *img);

    /**
     * @brief Draw this image to a pixel buffer.
     *
     * Note that this function uses the top left of the pixel buffer
     * as the origin.  The pixel buffer may still be modified if this
     * function fails.  This function may only be called once per
     * image.  This will only work on RGBX or RGBA pixel buffers.
     *
     * @param img This image.
     * @param pbuf The target pixel buffer.
     * @param x The target X offset for the image upper left.
     * @param y The target Y offset for the image upper left.
     * @param err On failure, the error.
     */
    int (*draw)(struct sg_image *img, struct sg_pixbuf *pbuf,
                int x, int y, struct sg_error **err);
};

/**
 * @brief Load an image from a file.
 *
 * @param path The path to the image to load.
 * @param pathlen The length of the path, in bytes.
 * @param err On failure, the error.
 * @return An image, or NULL for failure.
 */
struct sg_image *
sg_image_file(const char *path, size_t pathlen, struct sg_error **err);

/**
 * @brief Load an image from a file buffer.
 *
 * @param data The file buffer containing the image data.
 * @param err On failure, the error.
 * @return An image, or NULL for failure.
 */
struct sg_image *
sg_image_buffer(struct sg_filedata *data, struct sg_error **err);

#ifdef __cplusplus
}
#endif
#endif
