#include "thread.h"

void
sg_lock_init(struct sg_lock *p)
{
    InitializeCriticalSection(&p->s);
}

void
sg_lock_destroy(struct sg_lock *p)
{
    DeleteCriticalSection(&p->s);
}

void
sg_lock_acquire(struct sg_lock *p)
{
    EnterCriticalSection(&p->s);
}

void
sg_lock_release(struct sg_lock *p)
{
    LeaveCriticalSection(&p->s);
}
