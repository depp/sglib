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
    struct sg_filedata *data;
    const char *darr[1];
    GLint larr[1], flag, loglen;
    char *errlog;
    GLuint shader;
    int r;

    r = sg_file_load(&data, path, strlen(path), 0,
                     "glsl", 1024 * 1024, NULL, NULL);
    if (r)
        abort();

    if (data->length > INT_MAX) {
        sg_logf(SG_LOG_ERROR, "%s: too long", path);
        return -1;
    }

    darr[0] = data->data;
    larr[0] = (GLint) data->length;
    shader = glCreateShader(type);
    glShaderSource(shader, 1, darr, larr);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &flag);
    if (flag)
        return shader;

    sg_logf(SG_LOG_ERROR, "%s: compilation failed", path);
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &loglen);
    if (loglen > 0) {
        errlog = malloc(loglen);
        if (errlog) {
            glGetShaderInfoLog(shader, loglen, NULL, errlog);
            sg_logs(SG_LOG_ERROR, errlog);
            free(errlog);
        }
    }
    glDeleteShader(shader);
    abort();
}

GLuint
load_program(const char *vertpath, const char *fragpath)
{
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

    sg_logf(SG_LOG_ERROR, "%s + %s: linking failed",
            vertpath, fragpath);
    glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &loglen);
    if (loglen <= 0) {
        errlog = malloc(loglen);
        if (errlog) {
            glGetProgramInfoLog(prog, loglen, NULL, errlog);
            sg_logs(SG_LOG_ERROR, errlog);
            free(errlog);
        }
    }
    abort();
}
