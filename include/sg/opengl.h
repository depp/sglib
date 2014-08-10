/* Copyright 2012-2013 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#ifndef SG_OPENGL_H
#define SG_OPENGL_H
#include "config.h"

/**
 * @file opengl.h
 *
 * @brief OpenGL.
 *
 * This file automatically includes the correct OpenGL header for the
 * platform and configuration.
 */

#if defined USE_OPENGL_GLEW || 1
# if defined USE_BUNDLED_GLEW
#  include "GL/glew.h"
# else
#  include <GL/glew.h>
# endif
#elif defined USE_OPENGL_PLATFORM
# if defined __APPLE__
#  include <OpenGL/gl.h>
#  include <OpenGL/glext.h>
# else
#  include <GL/gl.h>
#  include <GL/glext.h>
# endif
#elif defined USE_OPENGL_PLATFORM_GL3
# if defined __APPLE__
#  include <OpenGL/gl3.h>
# else
#  include <GL/gl3.h>
# endif
#else
# error "No OpenGL implementation configured."
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Check for OpenGL errors and log them.
 *
 * @param where A format string for error locations.
 * @return Zero if there were no errors, nonzero otherwise.
 */
int
sg_opengl_checkerror(const char *where, ...);

#ifdef __cplusplus
}
#endif
#endif
