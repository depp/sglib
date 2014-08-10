/* Copyright 2013 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "defs.h"
#include "sg/file.h"
#include "sg/log.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

GLuint
load_shader(const char *path, GLenum type)
{
    struct sg_buffer *buf;
    const char *darr[1];
    GLint larr[1], flag, loglen;
    struct sg_logger *log;
    char *errlog;
    GLuint shader;

    buf = sg_file_get(path, strlen(path), SG_RDONLY,
                      "glsl", 1024 * 1024, NULL);
    if (!buf)
        abort();

    if (buf->length > INT_MAX) {
        log = sg_logger_get("shader");
        sg_logf(log, SG_LOG_ERROR, "%s: too long", path);
        return -1;
    }

    darr[0] = buf->data;
    larr[0] = (GLint) buf->length;
    shader = glCreateShader(type);
    glShaderSource(shader, 1, darr, larr);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &flag);
    if (flag)
        return shader;

    log = sg_logger_get("shader");
    sg_logf(log, SG_LOG_ERROR, "%s: compilation failed", path);
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &loglen);
    if (loglen > 0) {
        errlog = malloc(loglen);
        if (errlog) {
            glGetShaderInfoLog(shader, loglen, NULL, errlog);
            sg_logs(log, SG_LOG_ERROR, errlog);
            free(errlog);
        }
    }
    glDeleteShader(shader);
    abort();
}

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
    sg_logf(log, SG_LOG_ERROR, "%s + %s: linking failed",
            vertpath, fragpath);
    glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &loglen);
    if (loglen <= 0) {
        errlog = malloc(loglen);
        if (errlog) {
            glGetProgramInfoLog(prog, loglen, NULL, errlog);
            sg_logs(log, SG_LOG_ERROR, errlog);
            free(errlog);
        }
    }
    abort();
}
