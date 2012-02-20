#include "dispatch.h"
#include "dispatch_impl.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

void
sg_dispatch_queue_push(struct sg_dispatch_queue *queue,
                      int priority, void *cxt, void (*func)(void *))
{
    struct sg_dispatch_task *tasks;
    int cprio;
    unsigned uprio, ctr, m, n, nn, depth, ndepth;

    ctr = queue->ctr++;
    cprio = priority;
    if (cprio > SG_DISPATCH_PRIO_MAX)
        cprio = SG_DISPATCH_PRIO_MAX;
    else if (cprio < SG_DISPATCH_PRIO_MIN)
        cprio = SG_DISPATCH_PRIO_MIN;
    uprio = ((unsigned) (cprio + 32768) << 16) | ((~ctr) & 0xffff);
    m = (~ctr) & 0x8000;

    tasks = queue->tasks;
    depth = queue->depth;
    n = queue->count;
    if (n >= (1u << depth) - 1) {
        ndepth = depth ? depth + 1 : 4;
        if (ndepth > 16)
            abort();
        tasks = realloc(tasks, sizeof(*tasks) * ((1u << ndepth) - 1));
        if (!tasks)
            goto nomem;
        queue->tasks = tasks;
        queue->depth = ndepth;
        depth = ndepth;
    }
    queue->count = n + 1;
    while (n) {
        nn = (n - 1) >> 1;
        if ((uprio ^ m) <= (tasks[nn].priority ^ m))
            break;
        memcpy(&tasks[n], &tasks[nn], sizeof(*tasks));
        n = nn;
    }
    tasks[n].priority = uprio;
    tasks[n].cxt = cxt;
    tasks[n].func = func;
    return;

nomem:
    abort();
}

void
sg_dispatch_queue_pop(struct sg_dispatch_queue *queue,
                      struct sg_dispatch_task *task)
{
    struct sg_dispatch_task *tasks;
    unsigned ntasks, n, m, p, p1, p2, nn, pp;

    ntasks = queue->count;
    assert(ntasks > 0);
    queue->count = --ntasks;
    tasks = queue->tasks;
    memcpy(task, &tasks[0], sizeof(*task));
    if (!ntasks)
        return;

    n = 0;
    m = (~queue->ctr) & 0x8000;
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
            memcpy(&tasks[n], &tasks[nn], sizeof(*task));
            n = nn;
        } else {
            nn = 2*n+1;
            if (nn < ntasks && (tasks[nn].priority ^ m) > p) {
                memcpy(&tasks[n], &tasks[nn], sizeof(*task));
                n = nn;
            }
            break;
        }
    }
    memcpy(&tasks[n], &tasks[ntasks], sizeof(*task));
}

void
sg_dispatch_task_exec(struct sg_dispatch_task *task)
{
    task->func(task->cxt);
}
