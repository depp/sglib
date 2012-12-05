/* Copyright 2012 Dietrich Epp <depp@zdome.net> */
#include "libpce/thread.h"

void
pce_lock_init(struct pce_lock *p)
{
    InitializeCriticalSection(&p->s);
}

void
pce_lock_destroy(struct pce_lock *p)
{
    DeleteCriticalSection(&p->s);
}

void
pce_lock_acquire(struct pce_lock *p)
{
    EnterCriticalSection(&p->s);
}

void
pce_lock_release(struct pce_lock *p)
{
    LeaveCriticalSection(&p->s);
}
