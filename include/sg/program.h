/* Copyright 2013 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#ifndef SG_PROGRAM_H
#define SG_PROGRAM_H
#include "sg/opengl.h"
#ifdef __cplusplus
extern "C" {
#endif

/* Try to link a program, returning 0 for success and nonzero for
   failure.  Logs errors.  */
int
sg_program_link(GLuint program, const char *name);

#ifdef __cplusplus
}
#endif
#endif
