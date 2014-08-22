/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#define _XOPEN_SOURCE 600

#include "sg/thread.h"
#include <errno.h>
#include <stdlib.h>

void
pce_lock_init(struct pce_lock *p)
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
pce_lock_destroy(struct pce_lock *p)
{
    int r;
    r = pthread_mutex_destroy(&p->m);
    if (r) goto err;
    return;

err:
    abort();
}

void
pce_lock_acquire(struct pce_lock *p)
{
    int r;
    r = pthread_mutex_lock(&p->m);
    if (r) goto err;
    return;

err:
    abort();
}

int
pce_lock_try(struct pce_lock *p)
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
pce_lock_release(struct pce_lock *p)
{
    int r;
    r = pthread_mutex_unlock(&p->m);
    if (r) goto err;
    return;

err:
    abort();
}

/* ======================================== */

void
pce_rwlock_init(struct pce_rwlock *p)
{
    pthread_rwlockattr_t lattr;
    int r;
    r = pthread_rwlockattr_init(&lattr);
    if (r) goto err;
    r = pthread_rwlockattr_setpshared(&lattr, PTHREAD_PROCESS_PRIVATE);
    if (r) goto err;
    r = pthread_rwlock_init(&p->l, &lattr);
    if (r) goto err;
    r = pthread_rwlockattr_destroy(&lattr);
    if (r) goto err;
    return;

err:
    abort();

}

void
pce_rwlock_destroy(struct pce_rwlock *p)
{
    int r;
    r = pthread_rwlock_destroy(&p->l);
    if (r) goto err;
    return;

err:
    abort();
}

void
pce_rwlock_wracquire(struct pce_rwlock *p)
{
    int r;
    r = pthread_rwlock_wrlock(&p->l);
    if (r) goto err;
    return;

err:
    abort();
}

int
pce_rwlock_wrtry(struct pce_rwlock *p)
{
    int r;
    r = pthread_rwlock_trywrlock(&p->l);
    if (r == EBUSY)
        return 0;
    if (r == 0)
        return 1;

    abort();
}

void
pce_rwlock_wrrelease(struct pce_rwlock *p)
{
    int r;
    r = pthread_rwlock_unlock(&p->l);
    if (r) goto err;
    return;

err:
    abort();
}

void
pce_rwlock_rdacquire(struct pce_rwlock *p)
{
    int r;
    r = pthread_rwlock_rdlock(&p->l);
    if (r) goto err;
    return;

err:
    abort();
}

int
pce_rwlock_rdtry(struct pce_rwlock *p)
{
    int r;
    r = pthread_rwlock_tryrdlock(&p->l);
    if (r == EBUSY)
        return 0;
    if (r == 0)
        return 1;

    abort();
}

void
pce_rwlock_rdrelease(struct pce_rwlock *p)
{
    pce_rwlock_wrrelease(p);
}

/* ======================================== */

void
pce_evt_init(struct pce_evt *p)
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
    r = pthread_cond_init(&p->c, NULL);
    if (r) goto err;
    p->is_signaled = 0;
    return;

err:
    abort();

}

void
pce_evt_destroy(struct pce_evt *p)
{
    int r;
    r = pthread_mutex_destroy(&p->m);
    if (r) goto err;
    r = pthread_cond_destroy(&p->c);
    if (r) goto err;
    return;

err:
    abort();
}

void
pce_evt_signal(struct pce_evt *p)
{
    int r;
    r = pthread_mutex_lock(&p->m);
    if (r) goto err;
    if (!p->is_signaled) {
        p->is_signaled = 1;
        r = pthread_cond_signal(&p->c);
        if (r) goto err;
    }
    r = pthread_mutex_unlock(&p->m);
    if (r) goto err;
    return;

err:
    abort();
}

void
pce_evt_wait(struct pce_evt *p)
{
    int r;
    r = pthread_mutex_lock(&p->m);
    if (r) goto err;
    while (!p->is_signaled) {
        r = pthread_cond_wait(&p->c, &p->m);
        if (r) goto err;
    }
    p->is_signaled = 0;
    r = pthread_mutex_unlock(&p->m);
    if (r) goto err;
    return;

err:
    abort();
}
