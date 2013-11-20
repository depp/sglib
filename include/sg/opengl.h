/* Copyright 2012-2013 Dietrich Epp.
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

/* Check for OpenGL errors and log them.  If there were no errors, then zero
   is returned.  If there were errors, a non-zero value is returned.  */
int
sg_opengl_checkerror(const char *where);

#endif
