#ifndef CLIENT_OPENGL_HPP
#define CLIENT_OPENGL_HPP

#if defined(__APPLE__)

/* Mac OS X */
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>

#else

#if defined(_WIN32)
#include <WTypes.h>
#include <wingdi.h>
#endif

/* GNU/Linux */
#include <GL/gl.h>
#include <GL/glu.h>

#endif

#endif
