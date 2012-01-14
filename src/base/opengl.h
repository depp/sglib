#ifndef IMPL_OPENGL_H
#define IMPL_OPENGL_H

#if defined(__APPLE__)

/* Mac OS X */
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>

#elif defined(_WIN32)

#include <WTypes.h>
#include <wingdi.h>
#include <GL/gl.h>
#include <GL/glu.h>

#define GL_CONSTANT_COLOR 0x8001
extern "C" {
    extern void (APIENTRY *glBlendColor)(GLclampf, GLclampf, GLclampf, GLclampf);
}

#else

/* GNU/Linux */
#include <GL/gl.h>
#include <GL/glu.h>

#endif

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
