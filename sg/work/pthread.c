/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
/*
  POSIX threads implementation of the generic work queue interface.
  This allocates threads up to the maximum, and lets threads expire if
  they wait too long without executing a task.
*/

/* this condition might as well be voodoo magic... */
#if !defined(__APPLE__)
# define _XOPEN_SOURCE 600
#endif

#include "work.h"
#include <errno.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
/* <unistd.h> necessary for _POSIX_TIMERS */
#include <unistd.h>

void
sg_workqueue_init(struct sg_workqueue *q)
{
    int r;
    pthread_mutexattr_t mattr;

    r = pthread_mutexattr_init(&mattr);
    if (r) goto err;
    r = pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_NORMAL);
    if (r) goto err;
    r = pthread_mutex_init(&q->impl.mutex, &mattr);
    if (r) goto err;
    r = pthread_mutexattr_destroy(&mattr);
    if (r) goto err;
    r = pthread_cond_init(&q->impl.cond, NULL);
    if (r) goto err;

    return;

err:
    abort();
}

void
sg_workqueue_lock(struct sg_workqueue *q)
{
    int r;
    r = pthread_mutex_lock(&q->impl.mutex);
    if (r) goto err;
    return;

err:
    abort();
}

void
sg_workqueue_unlock(struct sg_workqueue *q)
{
    int r;
    r = pthread_mutex_unlock(&q->impl.mutex);
    if (r) goto err;
    return;

err:
    abort();
}

/* Get the current time.  Note that Mac OS X does not have
   clock_gettime.  */
static void
sg_workqueue_gettime(struct timespec *ts)
{
    int r;
#if defined(_POSIX_TIMERS) && _POSIX_TIMERS > 0
#if defined(_CLOCK_MONOTONIC)
    r = clock_gettime(CLOCK_MONOTONIC, ts);
#else
    r = clock_gettime(CLOCK_REALTIME, ts);
#endif
    if (r) goto err;
#else
    struct timeval tv;
    r = gettimeofday(&tv, NULL);
    if (r) goto err;
    ts->tv_sec = tv.tv_sec;
    ts->tv_nsec = tv.tv_usec * 1000;
#endif
    return;

err:
    abort();
}

/* Main worker thread loop.  */
static void *
sg_workqueue_run(void *ptr)
{
    struct sg_workqueue *q = ptr;
    void *task = NULL;
    struct timespec ts;
    int r, timedout = 0;

    task = malloc(q->tasksize);
    if (!task) goto err;

    r = pthread_mutex_lock(&q->impl.mutex);
    if (r) goto err;
    while (1) {
        if (q->dequeue(q, task)) {
            r = pthread_mutex_unlock(&q->impl.mutex);
            if (r) goto err;
            q->exec(task);
            r = pthread_mutex_lock(&q->impl.mutex);
            if (r) goto err;
        } else {
            if (timedout)
                break;
            sg_workqueue_gettime(&ts);
            ts.tv_sec += 1;
            q->thread_idle += 1;
            r = pthread_cond_timedwait(&q->impl.cond, &q->impl.mutex, &ts);
            q->thread_idle -= 1;
            if (r) {
                if (r == ETIMEDOUT)
                    timedout = 1;
                else
                    goto err;
            } else {
                timedout = 0;
            }
        }
    }
    q->thread_count -= 1;
    r = pthread_mutex_unlock(&q->impl.mutex);
    if (r) goto err;
    free(task);
    return NULL;

err:
    abort();
}

void
sg_workqueue_wake(struct sg_workqueue *q)
{
    pthread_t thread;
    pthread_attr_t attr;
    int r;
    if (q->thread_idle) {
        r = pthread_cond_signal(&q->impl.cond);
        if (r) goto err;
    } else if (q->thread_count < q->thread_max) {
        r = pthread_attr_init(&attr);
        if (r) goto err;
        r = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        if (r) goto err;
        r = pthread_create(&thread, &attr, sg_workqueue_run, q);
        if (r) goto err;
        r = pthread_attr_destroy(&attr);
        if (r) goto err;
        q->thread_count += 1;
    }
    return;

err:
    abort();
}
