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
#define SG_HAVE_RWLOCK 1

struct sg_lock {
    pthread_mutex_t m;
};

struct sg_rwlock {
    pthread_rwlock_t l;
};

#elif defined(_WIN32)

struct sg_lock {
    CRITICAL_SECTION s;
};

/* FIXME: Windows Vista has a RW lock
   (and condition variables, and everything else that is good...) */

#else
#error "No threading implementation"
#endif

/* ========== Simple locks ========== */

void
sg_lock_init(struct sg_lock *p);

void
sg_lock_destroy(struct sg_lock *p);

void
sg_lock_acquire(struct sg_lock *p);

/* Try to acquire the lock.  Do not block.  Return 1 if successful, 0
   if failed.  */
int
sg_lock_try(struct sg_lock *p);

void
sg_lock_release(struct sg_lock *p);

/* ========== RW locks ========== */
#if defined(SG_HAVE_RWLOCK) && SG_HAVE_RWLOCK

void
sg_rwlock_init(struct sg_rwlock *p);

void
sg_rwlock_destroy(struct sg_rwlock *p);

void
sg_rwlock_wracquire(struct sg_rwlock *p);

/* Try to acquire the lock.  Do not block.  Return 1 if successful, 0
   if failed.  */
int
sg_rwlock_wrtry(struct sg_rwlock *p);

void
sg_rwlock_wrrelease(struct sg_rwlock *p);

void
sg_rwlock_rdacquire(struct sg_rwlock *p);

/* Try to acquire the lock.  Do not block.  Return 1 if successful, 0
   if failed.  */
int
sg_rwlock_rdtry(struct sg_rwlock *p);

void
sg_rwlock_rdrelease(struct sg_rwlock *p);

#else

#define sg_rwlock sg_lock
#define sg_rwlock_init sg_lock_init
#define sg_rwlock_destroy sg_lock_destroy
#define sg_rwlock_wracquire sg_lock_acquire
#define sg_rwlock_wrtry sg_lock_try
#define sg_rwlock_wrrelease sg_lock_release
#define sg_rwlock_rdacquire sg_lock_acquire
#define sg_rwlock_rdtry sg_lock_try
#define sg_rwlock_rdrelease sg_lock_release

#endif

#ifdef __cplusplus
}
#endif
#endif
