/* Copyright 2012-2013 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include <stdio.h>
#include "sg/thread.h"

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

void
sg_evt_init(struct sg_evt *p)
{
    p->e = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (!p->e)
        abort();
}

void
sg_evt_destroy(struct sg_evt *p)
{
    CloseHandle(p->e);
    p->e = NULL;
}

void
sg_evt_signal(struct sg_evt *p)
{
    BOOL r = SetEvent(p->e);
    if (!r)
        abort();
}

void
sg_evt_wait(struct sg_evt *p)
{
    DWORD r = WaitForSingleObject(p->e, INFINITE);
    if (r)
        abort();
}
