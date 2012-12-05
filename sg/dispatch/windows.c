/* Copyright 2012 Dietrich Epp <depp@zdome.net> */
#include "dispatch.h"
#include "dispatch_impl.h"
#include <Windows.h>

struct sg_dispatch_windows {
    DWORD tls; /* TLS key for retrieving thread info.  */
    HANDLE evt; /* Event for waking up threads.  */
    CRITICAL_SECTION cs; /* Lock for this structure.  */

    /* Current and maximum number of threads for each type.
       The current number might temporarily exceed the maximum
       when threads switch type.  */
    unsigned active[2];
    unsigned max[2];

    /* Queue for each type */
    struct sg_dispatch_queue queue[2];

    /* Number of threads waiting for a task to become
       available.  This includes newly created threads.  */
    unsigned waiting;

    /* Number of threads which are known to be awake.  */
    unsigned awake;
};

struct sg_dispatch_windows_local {
    int queueidx;
};

static struct sg_dispatch_windows *sg_dispatch_windows;

static DWORD
sg_dispatch_async_ncpu(void)
{
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    return info.dwNumberOfProcessors;
}

void
sg_dispatch_async_init(void)
{
    struct sg_dispatch_windows *p;
    DWORD nproc;

    p = calloc(1, sizeof(*p));
    if (!p) goto nomem;

    p->tls = TlsAlloc();
    if (p->tls == TLS_OUT_OF_INDEXES) goto err;
    p->evt = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (p->evt == NULL) goto err;
    InitializeCriticalSection(&p->cs);

    nproc = sg_dispatch_async_ncpu();
    if (nproc <= 1)
        p->max[0] = 1;
    else if (nproc >= 8)
        p->max[0] = 8;
    else
        p->max[0] = (unsigned) nproc;
    p->max[1] = 1;

    sg_dispatch_windows = p;
    return;

nomem:
    abort();

err:
    abort();
}

/* Get the queue index for a given type.  */
static int
sg_dispatch_windows_idxfromtype(sg_dispatch_type_t type)
{
    return type == SG_DISPATCH_NORMAL ? 0 : 1;
}

/* Worker thread loop.  */
static DWORD WINAPI
sg_dispatch_windows_exec(void *arg);

/* Wake a waiting thread or spawn a new one if necessary.  Caller must
   hold mutex.  */
static void
sg_dispatch_windows_wake(struct sg_dispatch_windows *p)
{
    BOOL r;
    HANDLE thread;
    if (!p->awake) {
        if (p->waiting) {
            r = SetEvent(p->evt);
            if (!r) goto err;
        } else {
            thread = CreateThread(NULL, 0, sg_dispatch_windows_exec,
                                  p, 0, NULL);
            if (thread == NULL) goto err;
            r = CloseHandle(thread);
            if (!r) goto err;
            p->waiting += 1;
        }
        p->awake = 1;
    }
    return;

err:
    abort();
}

static DWORD WINAPI
sg_dispatch_windows_exec(void *arg)
{
    struct sg_dispatch_windows *p = arg;
    struct sg_dispatch_windows_local local;
    struct sg_dispatch_task task;
    int i, timedout = 0;
    DWORD dw;
    BOOL r;
    HRESULT hr;

    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) goto err;

    r = TlsSetValue(p->tls, &local);
    if (!r) goto err;
    EnterCriticalSection(&p->cs);
    p->awake = 0;
    p->waiting -= 1;
    while (1) {
        for (i = 0; i < 2; ++i) {
            if (p->active[i] < p->max[i] && p->queue[i].count)
                break;
        }
        if (i < 2) {
            timedout = 0;
            sg_dispatch_queue_pop(&p->queue[i], &task);
            local.queueidx = i;
            p->active[i] += 1;
            for (; i < 2; ++i) {
                if (p->active[i] < p->max[i] && p->queue[i].count) {
                    sg_dispatch_windows_wake(p);
                    break;
                }
            }
            LeaveCriticalSection(&p->cs);

            sg_dispatch_task_exec(&task);

            EnterCriticalSection(&p->cs);
            p->active[local.queueidx] -= 1;
        } else {
            if (timedout)
                break;
            p->waiting += 1;
            LeaveCriticalSection(&p->cs);

            dw = WaitForSingleObject(p->evt, 1000);

            EnterCriticalSection(&p->cs);
            p->waiting -= 1;
            switch (dw) {
            case WAIT_OBJECT_0:
                timedout = 0;
                break;

            case WAIT_TIMEOUT:
                timedout = 1;
                break;

            default:
                LeaveCriticalSection(&p->cs);
                goto err;
            }
            p->awake = 0;
        }
    }
    LeaveCriticalSection(&p->cs);
    if (hr == S_OK)
        CoUninitialize();
    return 0;

err:
    abort();
    return 0;
}

void
sg_dispatch_async_queue(sg_dispatch_type_t type, int priority,
                        void *cxt, void (*func)(void *))
{
    struct sg_dispatch_windows *p;
    int i;
    p = sg_dispatch_windows;
    if (!p) goto err;

    EnterCriticalSection(&p->cs);
    i = sg_dispatch_windows_idxfromtype(type);
    sg_dispatch_queue_push(&p->queue[i], priority, cxt, func);
    if (p->queue[i].count == 1 && p->active[i] < p->max[i])
        sg_dispatch_windows_wake(p);
    LeaveCriticalSection(&p->cs);
    return;

err:
    abort();
}

void
sg_dispatch_async_settype(sg_dispatch_type_t type)
{
    struct sg_dispatch_windows *p;
    struct sg_dispatch_windows_local *local;
    int i, j;
    p = sg_dispatch_windows;
    if (!p) goto fail;
    local = TlsGetValue(p->tls);
    if (!local) goto fail;
    i = sg_dispatch_windows_idxfromtype(type);
    j = local->queueidx;
    if (i == j)
        return;

    EnterCriticalSection(&p->cs);
    local->queueidx = j;
    p->active[i] += 1;
    p->active[j] -= 1;
    if (p->active[j] == p->max[j] - 1 && p->queue[j].count)
        sg_dispatch_windows_wake(p);
    LeaveCriticalSection(&p->cs);
    return;

fail:
    abort();
}
