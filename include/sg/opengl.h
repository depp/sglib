/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#ifndef SG_OPENGL_H
#define SG_OPENGL_H
#include "config.h"

#if defined USE_BUNDLED_GLEW
# include "GL/glew.h"
#else
# include <GL/glew.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct sg_error;

extern const struct sg_error_domain SG_ERROR_OPENGL;

void
sg_error_opengl(struct sg_error **err, GLenum code);

/**
 * @brief Get the name for an OpenGL error code.
 *
 * The return value may or may not alias the buffer passed in.
 */
const char *
sg_error_openglname(char buf[12], GLenum code);

#ifdef __cplusplus
}
#endif
#endif
