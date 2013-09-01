/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#ifndef SG_TEXTURE_H
#define SG_TEXTURE_H
#include "libpce/attribute.h"
#include "sg/opengl.h"
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
struct sg_error;

/**
 * @file sg/texture.h
 *
 * OpenGL texture loading.
 *
 * Textures should only be accessed from the rendering thread.
 */

/**
 * @brief Texture.
 *
 * All fields should be treated as read-only.
 *
 * Ordinary 2D textures are loaded with the top left corner at
 * `(0,0)`, and the bottom right corner at `(iwidth,iheight)`.  The
 * texture size is normally rounded up to the nearest power of two,
 * making the size `(twidth,theight)`.
 */
struct sg_texture {
    /**
     * @private @brief Reference count.
     */
    unsigned refcount;

    /**
     * @private @brief Pointer to texture loading state for this
     * texture.
     * 
     * This field should not be accessed at all without the proper
     * lock.  It is @c NULL if the texture is not currently being
     * loaded.
     */
    struct sg_texture_load *load;

    /**
     * @brief Path to texture.
     */
    const char *path;

    /**
     * @brief OpenGL texture name.
     *
     * This is 0 if the texture is not loaded.
     */
    GLuint texnum;

    unsigned iwidth;  /**< @brief Image width.  */
    unsigned iheight; /**< @brief Image height.  */
    unsigned twidth;  /**< @brief Texture width.  */
    unsigned theight; /**< @brief Texture height.  */
};

/**
 * @brief Initialize the texture subsystem.
 */
void
sg_texture_init(void);

/**
 * @private
 * @brief Free a texture.
 */
void
sg_texture_free_(struct sg_texture *tp);

/**
 * @brief Increment texture reference count.
 */
PCE_INLINE void
sg_texture_incref(struct sg_texture *tp)
{
    tp->refcount += 1;
}

/**
 * @brief Decrement texture reference count.
 */
PCE_INLINE void
sg_texture_decref(struct sg_texture *tp)
{
    tp->refcount -= 1;
    if (!tp->refcount)
        sg_texture_free_(tp);
}

/**
 * @brief Create a texture from the image at the given path.
 *
 * @param path Path to the image.
 *
 * @param pathlen Length of the path
 *
 * @param err On failure, the error.
 *
 * @return On success, a new reference to a texture, possibly an
 * existing texture object.  This call should be paired with
 * sg_texture_decref().  On failure, returns @c NULL.
 */
struct sg_texture *
sg_texture_file(const char *path, size_t pathlen,
                struct sg_error **err);

#ifdef __cplusplus
}
#endif
#endif
