#include "dispatch.h"
#include "thread.h"
#include <stdlib.h>
#include <string.h>

struct sg_dispatch_callback {
    void *cxt;
    void (*func)(void *);
};

struct sg_dispatch_squeue {
    unsigned alloc;
    unsigned count;
    struct sg_dispatch_callback *queue;
};

struct sg_dispatch_sync {
    struct sg_dispatch_squeue cur, old;
    struct sg_lock lock;
};

static struct sg_dispatch_sync sg_dispatch_sync;

void
sg_dispatch_sync_init(void)
{
    sg_lock_init(&sg_dispatch_sync.lock);
}

void
sg_dispatch_sync_queue(void *cxt, void (*func)(void *))
{
    struct sg_dispatch_sync *m = &sg_dispatch_sync;
    unsigned nalloc, n;
    struct sg_dispatch_callback *queue;
    sg_lock_acquire(&m->lock);
    queue = m->cur.queue;
    n = m->cur.count;
    if (n >= m->cur.alloc) {
        nalloc = m->cur.alloc ? m->cur.alloc * 2 : 4;
        queue = realloc(queue, sizeof(*queue) * nalloc);
        if (!queue)
            goto err;
        m->cur.alloc = nalloc;
        m->cur.queue = queue;
    }
    queue[n].cxt = cxt;
    queue[n].func = func;
    m->cur.count = n + 1;
    sg_lock_release(&m->lock);
    return;

err:
    abort();
}

void
sg_dispatch_sync_run(void)
{
    struct sg_dispatch_sync *m = &sg_dispatch_sync;
    struct sg_dispatch_squeue q;
    struct sg_dispatch_callback *p, *e;

    sg_lock_acquire(&m->lock);
    memcpy(&q, &m->cur, sizeof(q));
    memcpy(&m->cur, &m->old, sizeof(q));
    memcpy(&m->old, &q, sizeof(q));
    m->cur.count = 0;
    sg_lock_release(&m->lock);

    for (p = q.queue, e = p + q.count; p != e; ++p)
        p->func(p->cxt);
}
