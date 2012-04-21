#define _XOPEN_SOURCE 600

#include "thread.h"
#include <errno.h>
#include <stdlib.h>

void
sg_lock_init(struct sg_lock *p)
{
    pthread_mutexattr_t mattr;
    int r;
    r = pthread_mutexattr_init(&mattr);
    if (r) goto err;
    r = pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_NORMAL);
    if (r) goto err;
    r = pthread_mutex_init(&p->m, &mattr);
    if (r) goto err;
    r = pthread_mutexattr_destroy(&mattr);
    if (r) goto err;
    return;

err:
    abort();
}

void
sg_lock_destroy(struct sg_lock *p)
{
    int r;
    r = pthread_mutex_destroy(&p->m);
    if (r) goto err;
    return;

err:
    abort();
}

void
sg_lock_acquire(struct sg_lock *p)
{
    int r;
    r = pthread_mutex_lock(&p->m);
    if (r) goto err;
    return;

err:
    abort();
}

int
sg_lock_try(struct sg_lock *p)
{
    int r;
    r = pthread_mutex_trylock(&p->m);
    if (r == EBUSY)
        return 0;
    if (r == 0)
        return 1;

    abort();
}

void
sg_lock_release(struct sg_lock *p)
{
    int r;
    r = pthread_mutex_unlock(&p->m);
    if (r) goto err;
    return;

err:
    abort();
}
