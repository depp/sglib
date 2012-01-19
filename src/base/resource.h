#ifndef BASE_RESOURCE_H
#define BASE_RESOURCE_H
#include "defs.h"
#ifdef __cplusplus
extern "C" {
#endif
/* A resource is something that can be loaded from disk or generated
   procedurally, and then unloaded.  It is assumed that resource
   loading is an expensive operation in terms of CPU or I/O, so
   resources are loaded asynchronously.  Resources may also be
   unloaded without warning.

   Resources are reference counted.  Multiple requests for "identical"
   resources may result in the same pointer.  */
struct sg_error;

typedef enum {
    SG_RSRC_TEXTURE,
    SG_RSRC_MODEL
} sg_resource_type_t;

enum {
    SG_RSRC_LOADED = 1U << 0
};

struct sg_resource {
    sg_resource_type_t type;
    unsigned refcount;
    unsigned flags;
    char *name;
};

static inline void
sg_resource_incref(struct sg_resource *rs)
{
    rs->refcount += 1;
}

static inline void
sg_resource_decref(struct sg_resource *rs)
{
    rs->refcount -= 1;
}

/* Find a resource with the given type and name, or return NULL if no
   such resource exists.  */
struct sg_resource *
sg_resource_find(sg_resource_type_t type, const char *name);

/* Add a new resource to the resource set.  This allows it to be found
   using sg_resource_find and loaded by the resource loader.  */
int
sg_resource_register(struct sg_resource *rs, struct sg_error **err);

/* Load all unloaded resources.  */
void
sg_resource_loadall(void);

/* Mark all resources of the given type as unloaded.  */
void
sg_resource_dirtytype(sg_resource_type_t type);

#ifdef __cplusplus
}
#endif
#endif
