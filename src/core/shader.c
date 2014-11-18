/* Copyright 2013-2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "sg/shader.h"
#include "sg/file.h"
#include "sg/error.h"
#include "sg/log.h"
#include <stdlib.h>

#define SG_SHADER_MAXSIZE (1024 * 64)

static const struct {
    char ext[5];
    GLenum type;
} SG_SHADER_EXTENSION[] = {
    { "comp", GL_COMPUTE_SHADER },
    { "vert", GL_VERTEX_SHADER },
    { "tesc", GL_TESS_CONTROL_SHADER },
    { "tese", GL_TESS_EVALUATION_SHADER },
    { "geom", GL_GEOMETRY_SHADER },
    { "frag", GL_FRAGMENT_SHADER }
};

const char *
sg_shader_file_extension(GLenum type)
{
    int i, n = sizeof(SG_SHADER_EXTENSION) / sizeof(*SG_SHADER_EXTENSION);
    for (i = 0; i < n; i++)
        if (SG_SHADER_EXTENSION[i].type == type)
            return SG_SHADER_EXTENSION[i].ext;
    return NULL;
}

GLuint
sg_shader_file(const char *path, size_t pathlen, GLenum type,
               struct sg_error **err)
{
    struct sg_filedata *data;
    const char *ext;
    int r;
    GLuint shader;
    ext = sg_shader_file_extension(type);
    if (!ext) {
        sg_error_invalid(err, __FUNCTION__, "type");
        return 0;
    }
    r = sg_file_load(&data, path, pathlen, 0, ext,
                     SG_SHADER_MAXSIZE, NULL, err);
    if (r)
        return 0;
    shader = sg_shader_buffer(data, type, err);
    sg_filedata_decref(data);
    return shader;
}

GLuint
sg_shader_buffer(struct sg_filedata *data, GLenum type,
                 struct sg_error **err)
{
    const char *src[1];
    char *log;
    GLuint shader;
    GLint srclen[1], flag, loglen;

    shader = glCreateShader(type);
    if (!shader) {
        sg_error_opengl(err);
        return 0;
    }
    src[0] = data->data;
    srclen[0] = data->length;
    glShaderSource(shader, 1, src, srclen);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &flag);
    if (!flag)
        sg_logf(SG_LOG_ERROR, "%s: Compilation failed.", data->path);
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &loglen);
    if (loglen > 1) {
        log = malloc(loglen + 1);
        if (!log) {
            sg_error_nomem(err);
            glDeleteShader(shader);
            return 0;
        }
        glGetShaderInfoLog(shader, loglen, NULL, log);
        log[loglen] = '\0';
        sg_logs(SG_LOG_WARN, log);
        free(log);
    }
    if (flag) {
        return shader;
    } else {
        sg_error_opengl(err);
        glDeleteShader(shader);
        return 0;
    }
}

int
sg_shader_link(GLuint program, const char *name, struct sg_error **err)
{
    char *log;
    GLint flag, loglen;
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &flag);
    if (!flag)
        sg_logf(SG_LOG_ERROR, "%s: Linking failed.", name);
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &loglen);
    if (loglen > 1) {
        log = malloc(loglen + 1);
        if (!log) {
            sg_error_nomem(err);
            return -1;
        }
        glGetProgramInfoLog(program, loglen, NULL, log);
        log[loglen] = '\0';
        sg_logs(SG_LOG_WARN, log);
        free(log);
    }
    if (flag) {
        return 0;
    } else {
        sg_error_opengl(err);
        return -1;
    }
}
