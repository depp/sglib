/* Copyright 2012-2013 Dietrich Epp <depp@zdome.net> */
#include <stdio.h>
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

void
pce_evt_init(struct pce_evt *p)
{
    p->e = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (!p->e)
        abort();
}

void
pce_evt_destroy(struct pce_evt *p)
{
    CloseHandle(p->e);
    p->e = NULL;
}

void
pce_evt_signal(struct pce_evt *p)
{
    BOOL r = SetEvent(p->e);
    if (!r)
        abort();
}

void
pce_evt_wait(struct pce_evt *p)
{
    DWORD r = WaitForSingleObject(p->e, INFINITE);
    if (r)
        abort();
}
