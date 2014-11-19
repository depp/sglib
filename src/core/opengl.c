/* Copyright 2012-2013 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "sg/error.h"
#include "sg/log.h"
#include "sg/opengl.h"
#include <stdarg.h>
#include <stdio.h>

struct sg_opengl_error {
    unsigned short code;
    const char name[30];
};

static const struct sg_opengl_error SG_ERROR_OPENGL_NAMES[] = {
    { 0x0500, "INVALID_ENUM" },
    { 0x0501, "INVALID_VALUE" },
    { 0x0502, "INVALID_OPERATION" },
    { 0x0503, "STACK_OVERFLOW" },
    { 0x0504, "STACK_UNDERFLOW" },
    { 0x0505, "OUT_OF_MEMORY" },
    { 0x0506, "INVALID_FRAMEBUFFER_OPERATION" },
    { 0x8031, "TABLE_TOO_LARGE" },
};

static void
sg_opengl_logerror(const char *where, GLenum code)
{
    int i, n = sizeof(SG_ERROR_OPENGL_NAMES) / sizeof(*SG_ERROR_OPENGL_NAMES);
    for (i = 0; i < n; i++) {
        if (SG_ERROR_OPENGL_NAMES[i].code == code) {
            sg_logf(SG_LOG_ERROR, "%s: OpenGL error GL_%s",
                    where, SG_ERROR_OPENGL_NAMES[i].name);
            return;
        }
    }
    sg_logf(SG_LOG_ERROR, "%s: OpenGL error 0x%04x",
            where, code);
}

int
sg_opengl_checkerror(const char *msg, ...)
{
    va_list ap;
    char where[256];
    GLenum error;

    error = glGetError();
    if (!error)
        return 0;

    va_start(ap, msg);
    where[0] = '\0';
#if defined(_MSC_VER)
    vsnprintf_s(where, sizeof(where), _TRUNCATE, msg, ap);
#else
    vsnprintf(where, sizeof(where), msg, ap);
#endif
    va_end(ap);

    do {
        sg_opengl_logerror(where, error);
        error = glGetError();
    } while (error);

    return 1;
}

const struct sg_error_domain SG_ERROR_OPENGL = { "opengl" };

void
sg_error_opengl(struct sg_error **err)
{
    sg_error_sets(err, &SG_ERROR_OPENGL, 0, "an OpenGL error occurred");
}
