/* Copyright 2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#ifndef SG_SHADER_H
#define SG_SHADER_H
#include "sg/opengl.h"
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
struct sg_error;
struct sg_filedata;

/**
 * @brief Get the preferred file extension for a shader type.
 *
 * @param type The shader type.
 * @return The preferred file extension, or NULL if the type is unknown.
 */
const char *
sg_shader_file_extension(GLenum type);

/**
 * @brief Load an OpenGL shader.
 *
 * @param path The path to the shader.
 * @param pathlen The length of the path, in bytes.
 * @param type The shader type.
 * @param err On failure, the error.
 * @return The shader, or 0 if the operation failed.
 */
GLuint
sg_shader_file(const char *path, size_t pathlen, GLenum type,
               struct sg_error **err);

/**
 * @brief Load an OpenGL shader.
 *
 * @param data The shader contents.
 * @param type The shader type.
 * @param err On failure, the error.
 * @return The shader, or 0 if the operation failed.
 */
GLuint
sg_shader_buffer(struct sg_filedata *data, GLenum type,
                 struct sg_error **err);

/**
 * @brief Link an OpenGL shader program.
 *
 * @param program The shader program to link.
 * @param name The name of the shader program for diagnostic messages.
 * @param err On failure, the error.
 * @return Zero for success, nonzero on failure.
 */
int
sg_shader_link(GLuint program, const char *name, struct sg_error **err);

#ifdef __cplusplus
}
#endif
#endif
