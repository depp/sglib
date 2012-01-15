#include "error.h"
#include "opengl.h"

const struct sg_error_domain SG_ERROR_OPENGL = { "opengl" };

void
sg_error_opengl(struct sg_error **err, GLenum code)
{
    sg_error_sets(err, &SG_ERROR_OPENGL, code,
                  (char *) gluErrorString(code));
}