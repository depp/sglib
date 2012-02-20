#ifndef BASE_THREAD_H
#define BASE_THREAD_H
#include "defs.h"
#if defined(HAVE_PTHREAD)
#include <pthread.h>
#elif defined(_WIN32)
#include <windows.h>
#endif
#ifdef __cplusplus
extern "C" {
#endif

#if defined(HAVE_PTHREAD)

struct sg_lock {
    pthread_mutex_t m;
};

#elif defined(_WIN32)

struct sg_lock {
    CRITICAL_SECTION s;
};

#else
#error "No threading implementation"
#endif

void
sg_lock_init(struct sg_lock *p);

void
sg_lock_destroy(struct sg_lock *p);

void
sg_lock_acquire(struct sg_lock *p);

void
sg_lock_release(struct sg_lock *p);

#ifdef __cplusplus
}
#endif
#endif
