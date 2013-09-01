/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "libpce/thread.h"
#include <stddef.h>

/*
  Private work queue implementation.

  There are two work queues in the program: a work queue for IO and a
  work queue for general CPU-bound tasks.
*/

#if defined(PCE_THREAD_PTHREAD)
#include <pthread.h>

struct sg_workqueue_impl {
    /* Mutex for queue */
    pthread_mutex_t mutex;

    /* Signal that queue is not empty */
    pthread_cond_t cond;
};

#elif defined(_WIN32)
#include <windows.h>

struct sg_workqueue_impl {
    /* Lock for queue */
    CRITICAL_SECTION cs;

    /* Event for waking up threads */
    HANDLE evt;
};

#else
#error "work queue not supported"
#endif

struct sg_workqueue {
    struct sg_workqueue_impl impl;

    /* Size of tasks in queue */
    size_t tasksize;

    /* Maximum number of threads working on queue */
    unsigned thread_max;

    /* Current number of threads working on queue */
    unsigned thread_count;

    /* Current number of threads waiting for work */
    unsigned thread_idle;

    /*
      Function for getting the front item in the queue.  This will
      only ever be called with the lock.  It should return 1 if a task
      was dequeued and 0 if the queue is empty.
    */
    int (*dequeue)(struct sg_workqueue *wq, void *taskptr);

    /*
      Function for executing an item in the queue.  This will be
      called without the lock.
    */
    void (*exec)(void *taskptr);
};

/*
  Initialize the work queue structure.  This only initializes the
  implementation-specific structures; other fields in the work queue
  structure should be set by the caller.
*/
void
sg_workqueue_init(struct sg_workqueue *q);

/*
  Lock the work queue.
*/
void
sg_workqueue_lock(struct sg_workqueue *q);

/*
  Unlock the work queue.
*/
void
sg_workqueue_unlock(struct sg_workqueue *q);

/*
  Ensure that at least one thread is awake and processing the work
  queue.  Caller must hold the lock.
*/
void
sg_workqueue_wake(struct sg_workqueue *q);
