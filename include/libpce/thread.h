/* Copyright 2012 Dietrich Epp <depp@zdome.net> */
#ifndef PCE_THREAD_H
#define PCE_THREAD_H

#if defined(__linux__) || defined(__APPLE__)
# define PCE_HAVE_RWLOCK 1
# define PCE_THREAD_PTHREAD 1
# include <pthread.h>
#elif defined(_WIN32)
# define PCE_THREAD_WINDOWS 1
# include <windows.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if defined(PCE_THREAD_PTHREAD)

struct pce_lock {
    pthread_mutex_t m;
};

struct pce_rwlock {
    pthread_rwlock_t l;
};

/* A simple event object.  It has two states: signaled and not
   signaled.  SIGNAL causes it to become signaled.  WAIT waits for it
   to become signaled, and atomically resets it on wake.  At most one
   thread will be awoken per SIGNAL.  */
struct pce_evt {
    pthread_mutex_t m;
    pthread_cond_t c;
    int is_signaled;
};

#elif defined(PCE_THREAD_WINDOWS)

struct pce_lock {
    CRITICAL_SECTION s;
};

/* FIXME: Windows Vista has a RW lock
   (and condition variables, and everything else that is good...) */

struct pce_evt {
    HANDLE e;
};

#else
# error "No threading implementation"
#endif

/* ========== Simple locks ========== */

void
pce_lock_init(struct pce_lock *p);

void
pce_lock_destroy(struct pce_lock *p);

void
pce_lock_acquire(struct pce_lock *p);

/* Try to acquire the lock.  Do not block.  Return 1 if successful, 0
   if failed.  */
int
pce_lock_try(struct pce_lock *p);

void
pce_lock_release(struct pce_lock *p);

/* ========== RW locks ========== */
#if defined(PCE_HAVE_RWLOCK) && PCE_HAVE_RWLOCK

void
pce_rwlock_init(struct pce_rwlock *p);

void
pce_rwlock_destroy(struct pce_rwlock *p);

void
pce_rwlock_wracquire(struct pce_rwlock *p);

/* Try to acquire the lock.  Do not block.  Return 1 if successful, 0
   if failed.  */
int
pce_rwlock_wrtry(struct pce_rwlock *p);

void
pce_rwlock_wrrelease(struct pce_rwlock *p);

void
pce_rwlock_rdacquire(struct pce_rwlock *p);

/* Try to acquire the lock.  Do not block.  Return 1 if successful, 0
   if failed.  */
int
pce_rwlock_rdtry(struct pce_rwlock *p);

void
pce_rwlock_rdrelease(struct pce_rwlock *p);

#else

#define pce_rwlock pce_lock
#define pce_rwlock_init pce_lock_init
#define pce_rwlock_destroy pce_lock_destroy
#define pce_rwlock_wracquire pce_lock_acquire
#define pce_rwlock_wrtry pce_lock_try
#define pce_rwlock_wrrelease pce_lock_release
#define pce_rwlock_rdacquire pce_lock_acquire
#define pce_rwlock_rdtry pce_lock_try
#define pce_rwlock_rdrelease pce_lock_release

#endif

/* ========== Events ========== */

void
pce_evt_init(struct pce_evt *p);

void
pce_evt_destroy(struct pce_evt *p);

void
pce_evt_signal(struct pce_evt *p);

void
pce_evt_wait(struct pce_evt *p);

#ifdef __cplusplus
}
#endif
#endif
