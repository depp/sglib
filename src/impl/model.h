#ifndef IMPL_MODEL_H
#define IMPL_MODEL_H
#include "resource.h"
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
struct sg_error;

struct sg_model {
    struct sg_resource r;

    double scale;

    /* Vertexes */
    unsigned vtype;
    unsigned vcount;
    void *vdata;

    /* Triangles */
    unsigned ttype;
    unsigned tcount;
    void *tdata;

    /* Lines */
    unsigned ltype;
    unsigned lcount;
    void *ldata;
};

int
sg_model_load(struct sg_model *mp, struct sg_error **err);

struct sg_model *
sg_model_new(const char *path, struct sg_error **err);

void
sg_model_draw(struct sg_model *mp);

#ifdef __cplusplus
}
#endif
#endif
