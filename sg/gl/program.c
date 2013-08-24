#include "sg/error.h"
#include "sg/log.h"
#include "sg/program.h"
#include <stdlib.h>

int
sg_program_link(GLuint program, const char *name)
{
    GLint flag, loglen;
    struct sg_logger *log;
    char *errlog;

    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &flag);
    if (flag)
        return 0;

    log = sg_logger_get("shader");
    sg_logf(log, LOG_ERROR, "%s: compilation failed", name);
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &loglen);
    if (loglen <= 0) {
        errlog = malloc(loglen);
        if (!errlog) {
            sg_error_nomem(NULL);
        } else {
            glGetProgramInfoLog(program, loglen, NULL, errlog);
            sg_logs(log, LOG_ERROR, errlog);
            free(errlog);
        }
    }
    return -1;
}
