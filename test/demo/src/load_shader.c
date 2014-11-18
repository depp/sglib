/* Copyright 2013-2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "defs.h"
#include "sg/entry.h"
#include "sg/shader.h"
#include <stdio.h>
#include <string.h>

GLuint
load_program(const char *vertpath, const char *fragpath)
{
    GLuint program, vertshader, fragshader;
    int r;
    char name[128];

    vertshader = sg_shader_file(
        vertpath, strlen(vertpath), GL_VERTEX_SHADER, NULL);
    if (!vertshader)
        return 0;
    fragshader = sg_shader_file(
        fragpath, strlen(fragpath), GL_FRAGMENT_SHADER, NULL);
    if (!fragshader) {
        glDeleteShader(vertshader);
        return 0;
    }

#if defined _WIN32
    _snprintf_s(name, sizeof(name), _TRUNCATE, "%s + %s", vertpath, fragpath);
#else
    snprintf(name, sizeof(name), "%s + %s", vertpath, fragpath);
#endif

    program = glCreateProgram();
    if (!program)
        sg_sys_abort("Could not create OpenGL shader program.");
    glAttachShader(program, vertshader);
    glAttachShader(program, fragshader);
    glDeleteShader(vertshader);
    glDeleteShader(fragshader);
    r = sg_shader_link(program, name, NULL);
    if (r < 0) {
        glDeleteProgram(program);
        return 0;
    }
    return program;
}
