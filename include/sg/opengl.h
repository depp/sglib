/* Copyright 2012 Dietrich Epp <depp@zdome.net> */
#ifndef SG_OPENGL_H
#define SG_OPENGL_H

#include <GL/glew.h>

#ifdef __cplusplus
extern "C" {
#endif

struct sg_error;

extern const struct sg_error_domain SG_ERROR_OPENGL;

void
sg_error_opengl(struct sg_error **err, GLenum code);

#ifdef __cplusplus
}
#endif
#endif
