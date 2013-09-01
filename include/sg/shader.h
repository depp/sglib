/* Copyright 2013 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#ifndef SG_SHADER_H
#define SG_SHADER_H
#include "libpce/attribute.h"
#include "sg/opengl.h"
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
struct sg_error;

/**
 * @file sg/shader.h
 *
 * OpenGL shader and program objects.
 *
 * Shaders and programs should only be accessed from the rendering
 * thread.
 */

/**
 * @brief Initialize the shader subsystem.
 */
void
sg_shader_init(void);

/**
 * @brief Shader.
 *
 * All fields should be treated as read-only.
 */
struct sg_shader {
    /**
     * @private @brief Reference count.
     */
    unsigned refcount;

    /**
     * @brief OpenGL shader name.
     *
     * This is 0 if the shader is not loaded.
     */
    GLuint shader;

    /**
     * @brief OpenGL shader type.
     */
    GLenum type;

    /**
     * @private @brief Pointer to shader loading state for this
     * shader.
     * 
     * This field should not be accessed at all without the proper
     * lock.  It is @c NULL if the shader is not currently being
     * loaded.
     */
    struct sg_shader_load *load;

    /**
     * @brief Path to shader.
     */
    const char *path;
};

/**
 * @private
 * @brief Free a shader.
 */
void
sg_shader_free_(struct sg_shader *sp);

/**
 * @brief Increment shader reference count.
 */
PCE_INLINE void
sg_shader_incref(struct sg_shader *sp)
{
    sp->refcount += 1;
}

/**
 * @brief Decrement shader reference count.
 */
PCE_INLINE void
sg_shader_decref(struct sg_shader *sp)
{
    sp->refcount -= 1;
    if (!sp->refcount)
        sg_shader_free_(sp);
}

/**
 * @brief Create a shader from the GLSL file at the given path.
 *
 * @param path Path to the GLSL file.
 *
 * @param pathlen Length of the path.
 *
 * @param type The shader type, e.g., @c GL_VERTEX_SHADER​ or @c
 * GL_FRAGMENT_SHADER​.
 *
 * @param err On failure, the error.
 *
 * @return On success, a new reference to a shader, possibly an
 * existing shader object.  This call should be paired with
 * sg_shader_decref().  On failure, returns @c NULL.
 */
struct sg_shader *
sg_shader_file(const char *path, size_t pathlen, GLenum type,
               struct sg_error **err);

#ifdef __cplusplus
}
#endif
#endif
