/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
/*
  Windows implementation of the generic work queue interface.  This
  allocates threads up to the maximum, and lets threads expire if they
  wait too long without executing a task.
*/

#include "work.h"

void
sg_workqueue_init(struct sg_workqueue *q)
{
    q->impl.evt = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (q->impl.evt == NULL) goto err;
    InitializeCriticalSection(&q->impl.cs);
    return;

err:
    abort();
}

void
sg_workqueue_lock(struct sg_workqueue *q)
{
    EnterCriticalSection(&q->impl.cs);
}

void
sg_workqueue_unlock(struct sg_workqueue *q)
{
    LeaveCriticalSection(&q->impl.cs);
}

static DWORD WINAPI
sg_workqueue_run(void *ptr)
{
    struct sg_workqueue *q = ptr;
    void *task;
    int timedout = 0;
    DWORD dw;
    HRESULT hr;

    task = malloc(q->tasksize);
    if (!task) goto err;

    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) goto err;

    EnterCriticalSection(&q->impl.cs);
    while (1) {
        if (q->dequeue(q, task)) {
            LeaveCriticalSection(&q->impl.cs);
            q->exec(task);
            EnterCriticalSection(&q->impl.cs);
        } else {
            if (timedout)
                break;
            q->thread_idle += 1;
            LeaveCriticalSection(&q->impl.cs);
            dw = WaitForSingleObject(q->impl.evt, 1000);
            EnterCriticalSection(&q->impl.cs);
            q->thread_idle -= 1;
            switch (dw) {
            case WAIT_OBJECT_0:
                timedout = 0;
                break;

            case WAIT_TIMEOUT:
                timedout = 1;
                break;

            default:
                LeaveCriticalSection(&q->impl.cs);
                goto err;
            }
        }
    }
    LeaveCriticalSection(&q->impl.cs);
    if (hr == S_OK)
        CoUninitialize();
    free(task);
    return 0;

err:
    abort();
    return 0;
}

void
sg_workqueue_wake(struct sg_workqueue *q)
{
    BOOL r;
    HANDLE thread;
    if (q->thread_idle) {
        r = SetEvent(q->impl.evt);
        if (!r) goto err;
    } else if (q->thread_count < q->thread_max) {
        thread = CreateThread(NULL, 0, sg_workqueue_run,
                              q, 0, NULL);
        if (thread == NULL) goto err;
        r = CloseHandle(thread);
        if (!r) goto err;
        q->thread_count += 1;
    }
    return;

err:
    abort();
}
