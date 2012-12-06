/* Copyright 2012 Dietrich Epp <depp@zdome.net> */
#ifndef SG_RESOURCE_H
#define SG_RESOURCE_H
#include "sg/defs.h"
#include "sg/dispatch.h"
#ifdef __cplusplus
extern "C" {
#endif
/* A resource is something that can be loaded from disk or generated
   procedurally, and then unloaded.  It is assumed that resource
   loading is an expensive operation in terms of CPU or I/O, so
   resources are loaded asynchronously.  Resources may also be
   unloaded without warning.

   Resources are reference counted.  When a resource's reference count
   drops to zero, the resource manager may unload and free it at some
   point in the future.

   Warning: This API is tricky.  Do not use this API without reading
   the documentation for it in full.  */
struct sg_error;
struct pce_strbuf;
struct sg_resource;

/* Error domain for when loading a resource was canceled.  */
extern const struct sg_error_domain SG_RESOURCE_CANCEL;

/* Information about a type of resource with callbacks for use by the
   resource manager.  The C version of a virtual function table.  */
struct sg_resource_type {
    /* Free the resource and all associated storage.  Called from
       sg_resource_updateall.  */
    void (*free)(struct sg_resource *rs);

    /* Called in an arbitrary (unknown) thread to load the resource.
       If an error occurs, err must be set.  This function may check
       to see if the state is PENDING_CANCEL and set an error using
       the sg_resource_canceled function above, or it may load a
       resource regardless of its cancellation.  The return value will
       be passed to 'load_finished', which executes on the main
       thread.  */
    void *(*load)(struct sg_resource *rs, struct sg_error **err);

    /* Called from sg_resource_updateall if and only if the 'load'
       function has finished.  The state will be LOADED if loading was
       successful, FAILED if it failed, and UNLOADED if loading was
       canceled.  The result is the result from 'load'.  */
    void (*load_finished)(struct sg_resource *rs, void *result);

    /* Write the name of this particular resource into the string
       buffer.  This should be prefixed with the type name followed by
       a colon.  */
    void (*get_name)(struct sg_resource *rs, struct pce_strbuf *buf);
};

/* Resource states.  User code is free to change the states to and
   from UNLOADED, LOADED, and FAILED.  User code should not change the
   state if it is LOADING or PENDING_CANCEL, and should not change to
   those states.

   The semantics of these states are not exactly described by their
   names.  For example, the FAILED state may be used to prevent the
   resource manager from loading a resource if it depends on another
   resource already being loaded.  */
typedef enum {
    /* Initial state for any resource where sg_resource_update has
       never been called.  The manager moves this to UNLOADED.  */
    SG_RSRC_INITIAL,

    /* Indicates that the resource should be loaded if its refcount is
       positive.  The manager moves this to LOADING.  User code may
       change from this state to LOADED or FAILED.  */
    SG_RSRC_UNLOADED,

    /* Indicates that the resource should be unloaded if its refcount
       is zero.  User code may move this to UNLOADED or FAILED.  */
    SG_RSRC_LOADED,

    /* Indicates that the resource manager should not take any action
       other than freeeing the resource.  User code may move this to
       UNLOADED or FAILED.  */
    SG_RSRC_FAILED,

    /* Indicates that a background task is currently loading the
       resource, and it should not be canceled.  The manager moves
       this to UNLOADED, LOADED, FAILED, or PENDING_CANCEL.  */
    SG_RSRC_LOADING,

    /* Indicaties that a background task is currently loading the
       resource, but it should be canceled.  The manager moves this to
       UNLOADED, LOADING, LOADED, or FAILED.  */
    SG_RSRC_PENDING_CANCEL
} sg_resource_state_t;

/* A resource.  With the exception of one field, this structure may
   only be modified from the main thread.  It is assumed that all
   properly aligned loads and stores are atomic, so it is safe to
   *read* any field from another thread.  After modifying the
   reference count or state, user code should call sg_resource_update.
   The sg_resource_update function should also be called when a
   resource is created.  */
struct sg_resource {
    /* Information about the resource type.  */
    const struct sg_resource_type *type;

    /* If the reference count is zero, the resource manager may unload
       or free the resource when sg_resource_updateall is called.  If
       the reference count is positive, the resource manager may
       create a background task to load the resource.  The resource
       manager may modify the reference count.  */
    unsigned short refcount;

    /* Private to the resource manager.  */
    unsigned short action;

    /* The current resource state.  This may be modified both by the
       resource manager and by user code, although user code may only
       the state to and from UNLOADED, LOADED, and FAILED states.  */
    sg_resource_state_t state;
};

/* Initialize the resource subsystem.  */
void
sg_resource_init(void);

/* Use as a pseudo-error result when loading a resource is
   canceled.  */
void
sg_resource_canceled(struct sg_error **err);

/* Update all resources, including pending state transitions based on
   the results from completed background tasks.  When this function is
   called, resources with a zero refcount may be unloaded and/or
   freed.  In fact, this is the only function which will free a
   resource.  This is called at the beginning of every frame, but may
   be called more often if it is known that resources with a zero
   refcount should be freed immediately.  */
void
sg_resource_updateall(void);

/* Update background tasks based on the current state and reference
   count.  The first time this is called, the state should be INITIAL.
   This will never free a resource.  */
void
sg_resource_update(struct sg_resource *rs);

/* Increment the reference count and call sg_resource_update.  */
void
sg_resource_incref(struct sg_resource *rs);

/* Decrement the reference count and call sg_resource_update.  */
void
sg_resource_decref(struct sg_resource *rs);

#ifdef __cplusplus
}
#endif
#endif
