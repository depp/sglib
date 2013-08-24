/* Copyright 2012 Dietrich Epp <depp@zdome.net> */
#include "sg/error.h"
#include "sg/opengl.h"
#include <stdio.h>

const struct sg_error_domain SG_ERROR_OPENGL = { "opengl" };

static const struct {
    GLenum code;
    const char *name;
} SG_ERROR_OPENGL_NAMES[] = {
    { 0x0500, "INVALID_ENUM​" },
    { 0x0501, "INVALID_VALUE​" },
    { 0x0502, "INVALID_OPERATION​" },
    { 0x0503, "OUT_OF_MEMORY​" },
    { 0x0506, "INVALID_FRAMEBUFFER_OPERATION​" },
    { 0x0503, "STACK_OVERFLOW​" },
    { 0x0504, "STACK_UNDERFLOW​" },
    { 0x8031, "TABLE_TOO_LARGE​1" },
    { 0, 0 }
};

void
sg_error_opengl(struct sg_error **err, GLenum code)
{
    int i;
    const char *name = NULL;
    for (i = 0; SG_ERROR_OPENGL_NAMES[i].code; i++) {
        if (SG_ERROR_OPENGL_NAMES[i].code == code) {
            name = SG_ERROR_OPENGL_NAMES[i].name;
            break;
        }
    }
    sg_error_sets(err, &SG_ERROR_OPENGL, code, name);
}

const char *
sg_error_openglname(char buf[12], GLenum code)
{
    int i;
    for (i = 0; SG_ERROR_OPENGL_NAMES[i].code; i++)
        if (SG_ERROR_OPENGL_NAMES[i].code == code)
            return SG_ERROR_OPENGL_NAMES[i].name;
#ifdef _MSC_VER
    sprintf(buf, "%#04x", (unsigned) code);
#else
    snprintf(buf, 12, "%#04x", (unsigned) code);
#endif
    return buf;
}
