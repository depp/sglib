#ifndef IMPL_MODEL_H
#define IMPL_MODEL_H
#include "resource.h"
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
struct sg_error;

enum {
    SG_MODEL_STATIC = 1U << 0
};

typedef enum {
    SG_MODEL_CUBE,
    SG_MODEL_PYRAMID
} sg_model_static_t;

struct sg_model {
    struct sg_resource r;

    unsigned flags;

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

struct sg_model *
sg_model_static(sg_model_static_t which, struct sg_error **err);

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
