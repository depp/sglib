/* Copyright 2012 Dietrich Epp <depp@zdome.net> */
/* Cooperative threading implementation of dispatch interface.  */
#include "impl.h"
#include "sg/dispatch.h"
#include <stdlib.h>

struct sg_dispatch_coop {
    struct sg_dispatch_queue queue;
};

static struct sg_dispatch_coop *sg_dispatch_coop;

/* TODO: implement this */
static void
sg_dispatch_coop_exec(void *ptr)
{
    struct sg_dispatch_coop *p;
}

static void
sg_dispatch_coop_init(void)
{
    struct sg_dispatch_coop *p;

    p = calloc(1, sizeof(*p));
    if (!p) goto nomem;

    sg_dispatch_coop = p;
    return;

nomem:
    abort();
}

void
sg_dispatch_queue(sg_dispatch_type_t type, int priority,
                  void *cxt, void (*func)(void *))
{
    struct sg_dispatch_coop *p;
    (void) type;
    p = sg_dispatch_coop;
    if (!p) {
        sg_dispatch_coop_init();
        p = sg_dispatch_coop;
    }
    sg_dispatch_queue_push(&p->queue, priority, cxt, func);
}

void
sg_dispatch_settype(sg_dispatch_type_t type)
{
    (void) type;
}
