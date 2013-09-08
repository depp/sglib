/* Copyright 2013 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "defs.h"
#include "sg/file.h"
#include "sg/log.h"
#include <stdlib.h>
#include <string.h>

GLuint
load_program(const char *vertpath, const char *fragpath)
{
    struct sg_logger *log;
    GLuint prog, vertshader, fragshader;
    GLint flag, loglen;
    char *errlog;

    vertshader = load_shader(vertpath, GL_VERTEX_SHADER);
    fragshader = load_shader(fragpath, GL_FRAGMENT_SHADER);
    prog = glCreateProgram();
    glAttachShader(prog, vertshader);
    glAttachShader(prog, fragshader);
    glDeleteShader(vertshader);
    glDeleteShader(fragshader);

    glLinkProgram(prog);
    glGetProgramiv(prog, GL_LINK_STATUS, &flag);
    if (flag)
        return prog;

    log = sg_logger_get("shader");
    sg_logf(log, LOG_ERROR, "%s + %s: linking failed",
            vertpath, fragpath);
    glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &loglen);
    if (loglen <= 0) {
        errlog = malloc(loglen);
        if (errlog) {
            glGetProgramInfoLog(prog, loglen, NULL, errlog);
            sg_logs(log, LOG_ERROR, errlog);
            free(errlog);
        }
    }
    abort();
}
