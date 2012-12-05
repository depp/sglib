/* Copyright 2012 Dietrich Epp <depp@zdome.net> */
#include "libpce/thread.h"
#include "sg/dispatch.h"
#include <stdlib.h>
#include <string.h>

struct sg_dispatch_callback {
    void *cxt;
    void (*func)(void *);
};

struct sg_dispatch_sqentry {
    struct sg_dispatch_callback cb;
    sg_dispatch_time_t time;
    int delay;
    int *excl;
};

struct sg_dispatch_sync {
    struct pce_lock lock;

    unsigned alloc;
    unsigned count;
    struct sg_dispatch_sqentry *queue;

    unsigned talloc;
    struct sg_dispatch_callback *tmp;
};

static struct sg_dispatch_sync sg_dispatch_sync;

void
sg_dispatch_sync_init(void)
{
    pce_lock_init(&sg_dispatch_sync.lock);
}

void
sg_dispatch_sync_queue(sg_dispatch_time_t time, int delay, int *excl,
                       void *cxt, void (*func)(void *))
{
    struct sg_dispatch_sync *m = &sg_dispatch_sync;
    unsigned nalloc, n;
    struct sg_dispatch_sqentry *queue;

    pce_lock_acquire(&m->lock);
    if (excl && *excl) {
        pce_lock_release(&m->lock);
        return;
    }
    queue = m->queue;
    n = m->count;
    if (n >= m->alloc) {
        nalloc = m->alloc ? m->alloc * 2 : 4;
        queue = realloc(queue, sizeof(*queue) * nalloc);
        if (!queue)
            goto err;
        m->alloc = nalloc;
        m->queue = queue;
    }
    queue[n].cb.cxt = cxt;
    queue[n].cb.func = func;
    queue[n].time = time;
    queue[n].delay = delay;
    queue[n].excl = excl;
    m->count = n + 1;
    if (excl)
        *excl = 1;
    pce_lock_release(&m->lock);

    return;

err:
    abort();
}

#include <stdio.h>

void
sg_dispatch_sync_run(sg_dispatch_time_t time)
{
    struct sg_dispatch_sync *m = &sg_dispatch_sync;
    struct sg_dispatch_sqentry *cq;
    struct sg_dispatch_callback *cb;
    unsigned i, j, qct, cbct, cballoc;

    pce_lock_acquire(&m->lock);
    cq = m->queue;
    qct = m->count;
    cbct = 0;
    cb = m->tmp;
    cballoc = m->talloc;
    for (i = 0, j = 0; i < qct; ++i) {
        if (cq[i].time == time) {
            if (cq[i].delay <= 0) {
                if (cbct >= cballoc) {
                    cballoc = cballoc ? 2 * cballoc : 4;
                    cb = realloc(cb, sizeof(*cb) * cballoc);
                    if (!cb)
                        goto err;
                    m->talloc = cballoc;
                    m->tmp = cb;
                }
                cb[cbct++] = cq[i].cb;
                if (cq[i].excl)
                    *cq[i].excl = 0;
                continue;
            } else {
                cq[i].delay -= 1;
            }
        }
        cq[j++] = cq[i];
    }
    m->count = j;
    pce_lock_release(&m->lock);

    for (i = 0; i < cbct; ++i)
        cb[i].func(cb[i].cxt);

    return;

err:
    abort();
}