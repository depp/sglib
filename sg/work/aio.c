/* Copyright 2012 Dietrich Epp <depp@zdome.net> */
#include "sg/aio.h"
#include "sg/error.h"
#include "work.h"
#include <stdlib.h>
#include <string.h>

struct sg_aio_request {
    struct sg_aio_request *prev, *next;
    int cancel;

    const char *path;
    size_t pathlen;
    int flags;
    const char *extensions;
    size_t maxsize;
    void *cxt;
    sg_aio_callback_t callback;
};

struct sg_aio_queue {
    struct sg_workqueue wq;

    /* Linked list of all asynchronous IO requests */
    struct sg_aio_request *first, *last;
};

static struct sg_aio_queue sg_aio_queue;

static int
sg_aio_dequeue(struct sg_workqueue *wq, void *taskptr)
{
    struct sg_aio_request *req;
    struct sg_aio_queue *q = (struct sg_aio_queue *) wq;

    req = q->first;
    if (!req)
        return 0;
    q->first = req->next;
    if (!q->first)
        q->last = NULL;
    *(struct sg_aio_request **) taskptr = req;
    return 1;
}

static void
sg_aio_exec(struct sg_workqueue *wq, void *taskptr)
{
    struct sg_aio_request *req = *(struct sg_aio_request **) taskptr;

    (void) wq;
    (void) req;
    /* FIXME do something */
}

void
sg_aio_init(void)
{
    struct sg_aio_queue *q = &sg_aio_queue;

    sg_workqueue_init(&q->wq);
    q->wq.tasksize = sizeof(struct sg_aio_request *);
    q->wq.thread_max = 1;
    q->wq.dequeue = sg_aio_dequeue;
    q->wq.exec = sg_aio_exec;
}

struct sg_aio_request *
sg_aio_request(const char *path, size_t pathlen, int flags,
               const char *extensions, size_t maxsize,
               void *cxt, sg_aio_callback_t callback,
               struct sg_error **err)
{
    struct sg_aio_queue *q;
    struct sg_aio_request *ioreq;
    size_t extlen;
    char *rpath, *rext;

    q = &sg_aio_queue;

    extlen = strlen(extensions);
    ioreq = malloc(sizeof(*ioreq) + pathlen + extlen + 2);
    if (!ioreq) {
        sg_error_nomem(err);
        return NULL;
    }
    rpath = (char *) (ioreq + 1);
    rext = rpath + pathlen + 1;

    memcpy(rpath, path, pathlen);
    rpath[pathlen] = '\0';
    memcpy(rext, extensions, extlen + 1);

    ioreq->path = rpath;
    ioreq->pathlen = pathlen;
    ioreq->flags = flags;
    ioreq->extensions = rext;
    ioreq->maxsize = maxsize;
    ioreq->cxt = cxt;
    ioreq->callback = callback;

    sg_workqueue_lock(&q->wq);

    ioreq->prev = q->last;
    ioreq->next = NULL;
    if (q->last)
        q->last->next = ioreq;
    else
        q->first = ioreq;
    q->last = ioreq;

    sg_workqueue_wake(&q->wq);

    sg_workqueue_unlock(&q->wq);

    return ioreq;
}
