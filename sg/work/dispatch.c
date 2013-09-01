/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "work.h"
#include "sg/defs.h"
#include "sg/dispatch.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#if defined(__linux__)
#include <unistd.h>

static int
sg_dispatch_ncpu(void)
{
    return sysconf(_SC_NPROCESSORS_ONLN);
}

#elif defined(__APPLE__)
#include <sys/types.h>
#include <sys/sysctl.h>

static int
sg_dispatch_ncpu(void)
{
    int mib[4], ncpu, r;
    size_t len;
    mib[0] = CTL_HW;
    mib[1] = HW_NCPU;
    len = sizeof(ncpu);
    r = sysctl(mib, 2, &ncpu, &len, NULL, 0);
    if (r < 0)
        return -1;
    return ncpu;
}

#elif defined(_WIN32)

static int
sg_dispatch_ncpu(void)
{
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    return info.dwNumberOfProcessors;
}

#else
#warning "cannot get number of CPUS on this system"

static int
sg_dispatch_ncpu(void)
{
    return 1;
}

#endif

struct sg_dispatch_task {
    /* Task priority.  The high 16 bits are the user assigned
       priority, and the low 16 bits are a counter.  Due to rollover,
       an older task will be prioritized lower if enough new tasks
       have been added since the older tasks.  The number of new tasks
       required is at least 2^15, so if rollover happens the old
       thread probably won't execute anyway.  */
    unsigned priority;

    /* Callback.  */
    void *cxt;
    void (*func)(void *);
};

struct sg_dispatch_system {
    struct sg_workqueue wq;

    /* The number of tasks in this queue.  */
    unsigned count;

    /* The base two logarithm of one plus the size of the array
       holding queued tasks.  This is also the depth of the heap.  */
    unsigned depth;

    /* Counter for tasks inserted, so tasks inserted later get
       prioritized lower relative to tasks at the same assigned
       priority level.  */
    unsigned ctr;

    /* Binary max heap for task priority queue.  */
    struct sg_dispatch_task *tasks;
};

static struct sg_dispatch_system sg_dispatch_system;

static int
sg_dispatch_dequeue(struct sg_workqueue *wq, void *taskptr)
{
    struct sg_dispatch_system *SG_RESTRICT q =
        (struct sg_dispatch_system *) wq;
    struct sg_dispatch_task *SG_RESTRICT tasks, *task = taskptr;
    unsigned ntasks, n, m, p, p1, p2, nn, pp;

    ntasks = q->count;
    if (!ntasks)
        return 0;

    q->count = --ntasks;
    tasks = q->tasks;
    *task = tasks[0];
    if (!ntasks)
        return 1;

    n = 0;
    m = (~q->ctr) & 0x8000;
    p = tasks[ntasks].priority ^ m;
    while (1) {
        if (2*n+2 < ntasks)  {
            p1 = tasks[2*n+1].priority ^ m;
            p2 = tasks[2*n+2].priority ^ m;
            if (p1 > p2) {
                nn = 2*n+1;
                pp = p1;
            } else {
                nn = 2*n+2;
                pp = p2;
            }
            if (p >= pp)
                break;
            tasks[n] = tasks[nn];
            n = nn;
        } else {
            nn = 2*n+1;
            if (nn < ntasks && (tasks[nn].priority ^ m) > p) {
                tasks[n] = tasks[nn];
                n = nn;
            }
            break;
        }
    }
    tasks[n] = tasks[ntasks];
    return 1;
}

static void
sg_dispatch_exec(void *taskptr)
{
    struct sg_dispatch_task *task = taskptr;
    task->func(task->cxt);
}

void
sg_dispatch_init(void)
{
    struct sg_dispatch_system *q = &sg_dispatch_system;
    sg_workqueue_init(&q->wq);
    q->wq.tasksize = sizeof(struct sg_dispatch_task);
    q->wq.thread_max = sg_dispatch_ncpu();
    q->wq.dequeue = sg_dispatch_dequeue;
    q->wq.exec = sg_dispatch_exec;
}

void
sg_dispatch_queue(int priority, void *cxt, void (*func)(void *))
{
    struct sg_dispatch_system *q = &sg_dispatch_system;
    struct sg_dispatch_task *SG_RESTRICT tasks;
    int cprio;
    unsigned uprio, ctr, m, n, nn, depth, ndepth;

    sg_workqueue_lock(&q->wq);

    ctr = q->ctr++;
    cprio = priority;
    if (cprio > SG_DISPATCH_PRIO_MAX)
        cprio = SG_DISPATCH_PRIO_MAX;
    else if (cprio < SG_DISPATCH_PRIO_MIN)
        cprio = SG_DISPATCH_PRIO_MIN;
    uprio = ((unsigned) (cprio + 32768) << 16) | ((~ctr) & 0xffff);
    m = (~ctr) & 0x8000;

    tasks = q->tasks;
    depth = q->depth;
    n = q->count;
    if (n >= (1u << depth) - 1) {
        ndepth = depth ? depth + 1 : 4;
        if (ndepth > 16)
            abort();
        tasks = realloc(tasks, sizeof(*tasks) * ((1u << ndepth) - 1));
        if (!tasks)
            goto nomem;
        q->tasks = tasks;
        q->depth = ndepth;
        depth = ndepth;
    }
    q->count = n + 1;
    while (n) {
        nn = (n - 1) >> 1;
        if ((uprio ^ m) <= (tasks[nn].priority ^ m))
            break;
        tasks[n] = tasks[nn];
        n = nn;
    }
    tasks[n].priority = uprio;
    tasks[n].cxt = cxt;
    tasks[n].func = func;

    sg_workqueue_wake(&q->wq);

    sg_workqueue_unlock(&q->wq);

    return;

nomem:
    abort();
}
