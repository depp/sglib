#ifndef IMPL_TEXTURE_H
#define IMPL_TEXTURE_H
#include "resource.h"
#ifdef __cplusplus
extern "C" {
#endif
struct sg_error;

enum {
    SG_TEX_LOADED = 1U << 0
};

struct sg_texture {
    struct sg_resource r;

    /* OpenGL texture name */
    unsigned texnum;

    unsigned flags;
    unsigned iwidth, iheight;
    unsigned twidth, theight;
};

/* Load the given texture into graphics memory.  The OpenGL context
   must be set.  */
int
sg_texture_load(struct sg_texture *tex, struct sg_error **err);

/* Create a texture from the image at the specified path.  May return
   a previously created texture.  */
struct sg_texture *
sg_texture_new(const char *path, struct sg_error **err);

#ifdef __cplusplus
}
#endif
#endif
