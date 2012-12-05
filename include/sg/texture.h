#ifndef BASE_TEXTURE_H
#define BASE_TEXTURE_H
#include "resource.h"
#ifdef __cplusplus
extern "C" {
#endif
struct sg_error;

struct sg_texture_image {
    struct sg_resource r;
    const char *path;

    /* OpenGL texture name */
    unsigned texnum;

    unsigned iwidth, iheight;
    unsigned twidth, theight;
};

/* Create a texture from the image at the specified path.  May return
   a previously created texture.  */
struct sg_texture_image *
sg_texture_image_new(const char *path, struct sg_error **err);

#ifdef __cplusplus
}
#endif
#endif
