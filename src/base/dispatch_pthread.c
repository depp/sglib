/* POSIX threads implementation of dispatch interface.  This allocates
   a number of threads to the CPU-bound tasks and a number of threads
   to disk-bound tasks.  Threads are created as necessary and expire
   if they wait too long without executing a task.  */
#define _XOPEN_SOURCE 600

#include "dispatch.h"
#include "dispatch_impl.h"
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

struct sg_dispatch_pthread {
    pthread_attr_t attr; /* Attributes for new threads.  */
    pthread_mutex_t mutex; /* Mutex for all dispatch structures.  */
    pthread_cond_t cond; /* Signal available tasks.  */
    pthread_key_t key; /* Key for retrieving thread info.  */

    /* Current and maximum number of threads for each type.  The
       current number might temporarily exceed the maximum when
       threads switch type.  */
    unsigned active[2];
    unsigned max[2];

    /* Queue for each type.  */
    struct sg_dispatch_queue queue[2];

    /* Number of threads waiting for a task to become available.  This
       includes newly created threads.  */
    unsigned waiting;

    /* Number of waiting threads which are known to be awake.  */
    unsigned awake;
};

struct sg_dispatch_pthread_local {
    int queueidx;
};

static struct sg_dispatch_pthread *sg_dispatch_pthread;

void
sg_dispatch_async_init(void)
{
    struct sg_dispatch_pthread *p;
    pthread_mutexattr_t mattr;
    int r;
    long nproc;
    p = calloc(1, sizeof(*p));
    if (!p) goto nomem;

    r = pthread_attr_init(&p->attr);
    if (r) goto err;
    r = pthread_attr_setdetachstate(&p->attr, PTHREAD_CREATE_DETACHED);
    if (r) goto err;
    r = pthread_mutexattr_init(&mattr);
    if (r) goto err;
    r = pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_NORMAL);
    if (r) goto err;
    r = pthread_mutex_init(&p->mutex, &mattr);
    if (r) goto err;
    r = pthread_mutexattr_destroy(&mattr);
    if (r) goto err;
    r = pthread_cond_init(&p->cond, NULL);
    if (r) goto err;
    r = pthread_key_create(&p->key, NULL);
    if (r) goto err;

    nproc = sysconf(_SC_NPROCESSORS_ONLN);
    if (nproc <= 1)
        p->max[0] = 1;
    else if (nproc >= 8)
        p->max[0] = 8;
    else
        p->max[0] = (unsigned) nproc;
    p->max[1] = 1;

    sg_dispatch_pthread = p;
    return;

nomem:
    abort();

err:
    abort();
}

/* Get the queue index for a given type.  */
static int
sg_dispatch_pthread_idxfromtype(sg_dispatch_type_t type)
{
    return type == SG_DISPATCH_NORMAL ? 0 : 1;
}

/* Worker thread loop.  */
static void *
sg_dispatch_pthread_exec(void *arg);

/* Wake a waiting thread or spawn a new one if necessary.  Caller must
   hold mutex.  */
static void
sg_dispatch_pthread_wake(struct sg_dispatch_pthread *p)
{
    int r;
    pthread_t thread;
    if (!p->awake) {
        if (p->waiting) {
            r = pthread_cond_signal(&p->cond);
            if (r) goto err;
        } else {
            r = pthread_create(&thread, &p->attr,
                               sg_dispatch_pthread_exec, p);
            if (r) goto err;
            p->waiting += 1;
        }
        p->awake = 1;
    }
    return;

err:
    abort();
}

/* Get the current time.  Note that Mac OS X does not have
   clock_gettime.  */
static void
sg_dispatch_pthread_gettime(struct timespec *ts)
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
    struct tv;
    r = gettimeofday(&tv, NULL);
    if (r) goto err;
    ts->tv_sec = tv.tv_sec;
    ts->tv_nsec = tv.tv_usec * 1000;
#endif
    return;

err:
    abort();
}

/* Worker thread loop */
static void *
sg_dispatch_pthread_exec(void *arg)
{
    struct sg_dispatch_pthread *p = arg;
    struct sg_dispatch_pthread_local local;
    struct sg_dispatch_task task;
    struct timespec ts;
    int i, r, timedout = 0;

    r = pthread_setspecific(p->key, &local);
    if (r) goto err;
    r = pthread_mutex_lock(&p->mutex);
    if (!r) goto err;
    p->awake = 0;
    p->waiting -= 1;
    while (1) {
        for (i = 0; i < 2; ++i) {
            if (p->active[i] < p->max[i] && p->queue[i].count)
                break;
        }
        if (i < 2) {
            timedout = 0;
            sg_dispatch_queue_pop(&p->queue[i], &task);
            local.queueidx = i;
            p->active[i] += i;
            for (; i < 2; ++i) {
                if (p->active[i] < p->max[i] && p->queue[i].count) {
                    sg_dispatch_pthread_wake(p);
                    break;
                }
            }
            r = pthread_mutex_unlock(&p->mutex);
            if (r) goto err;

            sg_dispatch_task_exec(&task);

            r = pthread_mutex_lock(&p->mutex);
            if (r) goto err;
            p->active[local.queueidx] -= 1;
        } else {
            if (timedout)
                break;
            sg_dispatch_pthread_gettime(&ts);
            ts.tv_sec += 1;
            p->waiting += 1;
            r = pthread_cond_timedwait(&p->cond, &p->mutex, &ts);
            p->waiting -= 1;
            if (r) {
                if (r == ETIMEDOUT)
                    timedout = 1;
                else
                    goto err;
            } else {
                timedout = 0;
            }
            p->awake = 0;
        }
    }
    r = pthread_mutex_unlock(&p->mutex);
    if (r) goto err;
    return NULL;

err:
    abort();
}

void
sg_dispatch_async_queue(sg_dispatch_type_t type, int priority,
                        void *cxt, void (*func)(void *))
{
    struct sg_dispatch_pthread *p;
    int i, r;
    p = sg_dispatch_pthread;
    if (!p) goto err;

    r = pthread_mutex_lock(&p->mutex);
    if (r) goto err;

    i = sg_dispatch_pthread_idxfromtype(type);
    sg_dispatch_queue_push(&p->queue[i], priority, cxt, func);
    if (p->queue[i].count == 1 && p->active[i] < p->max[i])
        sg_dispatch_pthread_wake(p);

    r = pthread_mutex_unlock(&p->mutex);
    if (r) goto err;
    return;

err:
    abort();
}

void
sg_dispatch_async_settype(sg_dispatch_type_t type)
{
    struct sg_dispatch_pthread *p;
    struct sg_dispatch_pthread_local *local;
    int i, j, r;
    p = sg_dispatch_pthread;
    if (!p) goto fail;
    local = pthread_getspecific(p->key);
    if (!local) goto fail;
    i = sg_dispatch_pthread_idxfromtype(type);
    j = local->queueidx;
    if (i == j)
        return;

    r = pthread_mutex_lock(&p->mutex);
    if (r) goto err;

    local->queueidx = j;
    p->active[i] += 1;
    p->active[j] -= 1;
    if (p->active[j] == p->max[j] - 1 && p->queue[j].count)
        sg_dispatch_pthread_wake(p);

    r = pthread_mutex_unlock(&p->mutex);
    if (r) goto err;
    return;

fail:
    abort();

err:
    abort();
}
