/* Copyright 2012-2013 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
/* Work dispatching system.  A callback may be issued to an
   asynchronous queue.  Callbacks may be executed at arbitrary times,
   possibly in other threads but possibly in the same thread.  */
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

#ifdef __cplusplus
}
#endif
#endif
