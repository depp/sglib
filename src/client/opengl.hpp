#ifndef CLIENT_OPENGL_HPP
#define CLIENT_OPENGL_HPP

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

#endif
