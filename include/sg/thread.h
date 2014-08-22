/* Copyright 2012-2013 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#ifndef SG_THREAD_H
#define SG_THREAD_H

/**
 * @file thread.h
 *
 * @brief Thread synchronization objects.
 */

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

#elif defined(DOXYGEN)

/**
 * @brief A lock, also known as a critical section.
 *
 * Recursive locking is not supported.
 */
struct pce_lock { };

/**
 * @brief A reader-writer lock.
 *
 * Can be locked by any number of readers, or by one writer.
 */
struct pce_rwlock { };

/**
 * @brief An event, or binary semaphore.
 *
 * The event has two states: signaled and not signaled.  Signaling the
 * event causes it to become signaled.  Waiting for the event waits
 * for it to become signaled, and atomically resets it on wake.  At
 * most one thread will be awoken per signal.
 */
struct pce_event { };

#else
# error "No threading implementation"
#endif

/* ========== Simple locks ========== */

/** @brief Initialize a lock structure.  */
void
pce_lock_init(struct pce_lock *p);

/** @brief Destroy a lock structure.  */
void
pce_lock_destroy(struct pce_lock *p);

/** @brief Acquire a lock, blocking if necessary.  */
void
pce_lock_acquire(struct pce_lock *p);

/** @brief Try to acquire a lock without blocking, and return nonzero
    if successful.  */
int
pce_lock_try(struct pce_lock *p);

/** @brief Release a lock.  */
void
pce_lock_release(struct pce_lock *p);

/* ========== RW locks ========== */

/** @brief Initialize a reader-writer lock.  */
void
pce_rwlock_init(struct pce_rwlock *p);

/** @brief Destroy a reader-writer lock.  */
void
pce_rwlock_destroy(struct pce_rwlock *p);

/** @brief Acquire a reader-writer lock for writing, blocking if
    necessary.  */
void
pce_rwlock_wracquire(struct pce_rwlock *p);

/** @brief Try to acquire a reader-writer lock for writing without
    blocking, and return nonzero if successful.  */
int
pce_rwlock_wrtry(struct pce_rwlock *p);

/** @brief Release a writing lock on a reader-writer lock.  */
void
pce_rwlock_wrrelease(struct pce_rwlock *p);

/** @brief Acquire a reader-writer lock for reading, blocking if
    necessary.  */
void
pce_rwlock_rdacquire(struct pce_rwlock *p);

/** @brief Try to acquire a reader-writer lock for reading without
    blocking, and return nonzero if successful.  */
int
pce_rwlock_rdtry(struct pce_rwlock *p);

/** @brief Release a reading lock on a reader-writer lock.  */
void
pce_rwlock_rdrelease(struct pce_rwlock *p);

#if !defined(PCE_HAVE_RWLOCK) && !defined(DOXYGEN)

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

/** @brief Initialize an event.  */
void
pce_evt_init(struct pce_evt *p);

/** @brief Destroy an event.  */
void
pce_evt_destroy(struct pce_evt *p);

/** @brief Signal an event.  */
void
pce_evt_signal(struct pce_evt *p);

/** @brief Wait until an event is signaled.  */
void
pce_evt_wait(struct pce_evt *p);

#ifdef __cplusplus
}
#endif
#endif
