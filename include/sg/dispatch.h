/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
/* Work dispatching system.  A callback may be issued to an
   asynchronous queue or the synchronous queue.  Callbacks in the
   synchronous queue are executed between frames in the same thread as
   the renderer.  Callbacks in asynchronous queues may be executed at
   arbitrary times, possibly in other threads but possibly in the same
   thread as the synchronous tasks.

   If a task requires any nontrivial amount of CPU expenditure or disk
   IO, it should go in an asynchronous queue.  */
#ifndef SG_DISPATCH_H
#define SG_DISPATCH_H
#ifdef __cplusplus
extern "C" {
#endif

/* Priority limits.  Tasks with higher priority (larger numbers)
   execute before tasks with lower priority (lower numbers).  */
enum {
    SG_DISPATCH_PRIO_MAX = 32767,
    SG_DISPATCH_PRIO_MIN = -32768
};

/* Initialize asynchronous task dispatching system.  */
void
sg_dispatch_init(void);

/* Add a task to the queue to be completed asynchronously.  */
void
sg_dispatch_queue(int priority, void *cxt, void (*func)(void *));

typedef enum {
    /* Callbacks before rendering the frame.  */
    SG_PRE_RENDER,

    /* Callbacks after rendering the frame.  */
    SG_POST_RENDER,

    /* Callbacks after all resources are loaded, before rendering a
       frame.  */
    SG_RESOURCES_LOADED
} sg_dispatch_time_t;

/* Initialize synchronous queue.  */
void
sg_dispatch_sync_init(void);

/* Add a callback to be executed on the main (rendering) thread.

   If excl is not NULL, then the callback will only be added if *excl
   is 0.  The valiue of *excl will be set to 1, and then set back to 0
   before the callback executes.  Changes to *excl will be made with
   the queue lock, so this can be used to ensure that a callback only
   gets added once per frame, even when it is being added from
   different threads.  */
void
sg_dispatch_sync_queue(sg_dispatch_time_t time, int delay, int *excl,
                       void *cxt, void (*func)(void *));

/* Run all callbacks currently scheduled on the main thread.  */
void
sg_dispatch_sync_run(sg_dispatch_time_t time);

/* Adjust the counter of the number of resources being loaded.  If the
   counter hits zero, then all SG_RESOURCES_LOADED callbacks will be
   queued.  */
void
sg_dispatch_resource_adjust(int amount);

#ifdef __cplusplus
}
#endif
#endif
