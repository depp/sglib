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
# define SG_HAVE_RWLOCK 1
# define SG_THREAD_PTHREAD 1
# include <pthread.h>
#elif defined(_WIN32)
# define SG_THREAD_WINDOWS 1
# include <windows.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if defined(SG_THREAD_PTHREAD)

struct sg_lock {
    pthread_mutex_t m;
};

struct sg_rwlock {
    pthread_rwlock_t l;
};

struct sg_evt {
    pthread_mutex_t m;
    pthread_cond_t c;
    int is_signaled;
};

#elif defined(SG_THREAD_WINDOWS)

struct sg_lock {
    CRITICAL_SECTION s;
};

/* FIXME: Windows Vista has a RW lock
   (and condition variables, and everything else that is good...) */

struct sg_evt {
    HANDLE e;
};

#elif defined(DOXYGEN)

/**
 * @brief A lock, also known as a critical section.
 *
 * Recursive locking is not supported.
 */
struct sg_lock { };

/**
 * @brief A reader-writer lock.
 *
 * Can be locked by any number of readers, or by one writer.
 */
struct sg_rwlock { };

/**
 * @brief An event, or binary semaphore.
 *
 * The event has two states: signaled and not signaled.  Signaling the
 * event causes it to become signaled.  Waiting for the event waits
 * for it to become signaled, and atomically resets it on wake.  At
 * most one thread will be awoken per signal.
 */
struct sg_evt { };

#else
# error "No threading implementation"
#endif

/* ========== Simple locks ========== */

/** @brief Initialize a lock structure.  */
void
sg_lock_init(struct sg_lock *p);

/** @brief Destroy a lock structure.  */
void
sg_lock_destroy(struct sg_lock *p);

/** @brief Acquire a lock, blocking if necessary.  */
void
sg_lock_acquire(struct sg_lock *p);

/** @brief Try to acquire a lock without blocking, and return nonzero
    if successful.  */
int
sg_lock_try(struct sg_lock *p);

/** @brief Release a lock.  */
void
sg_lock_release(struct sg_lock *p);

/* ========== RW locks ========== */

/** @brief Initialize a reader-writer lock.  */
void
sg_rwlock_init(struct sg_rwlock *p);

/** @brief Destroy a reader-writer lock.  */
void
sg_rwlock_destroy(struct sg_rwlock *p);

/** @brief Acquire a reader-writer lock for writing, blocking if
    necessary.  */
void
sg_rwlock_wracquire(struct sg_rwlock *p);

/** @brief Try to acquire a reader-writer lock for writing without
    blocking, and return nonzero if successful.  */
int
sg_rwlock_wrtry(struct sg_rwlock *p);

/** @brief Release a writing lock on a reader-writer lock.  */
void
sg_rwlock_wrrelease(struct sg_rwlock *p);

/** @brief Acquire a reader-writer lock for reading, blocking if
    necessary.  */
void
sg_rwlock_rdacquire(struct sg_rwlock *p);

/** @brief Try to acquire a reader-writer lock for reading without
    blocking, and return nonzero if successful.  */
int
sg_rwlock_rdtry(struct sg_rwlock *p);

/** @brief Release a reading lock on a reader-writer lock.  */
void
sg_rwlock_rdrelease(struct sg_rwlock *p);

#if !defined(SG_HAVE_RWLOCK) && !defined(DOXYGEN)

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

/* ========== Events ========== */

/** @brief Initialize an event.  */
void
sg_evt_init(struct sg_evt *p);

/** @brief Destroy an event.  */
void
sg_evt_destroy(struct sg_evt *p);

/** @brief Signal an event.  */
void
sg_evt_signal(struct sg_evt *p);

/** @brief Wait until an event is signaled.  */
void
sg_evt_wait(struct sg_evt *p);

#ifdef __cplusplus
}
#endif
#endif
