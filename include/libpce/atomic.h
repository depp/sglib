/* Copyright 2012-2013 Dietrich Epp <depp@zdome.net> */
#ifndef PCE_ATOMIC_H
#define PCE_ATOMIC_H
#include "libpce/arch.h"
#include "libpce/attribute.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file atomic.h
 *
 * @brief Atomic operations.
 *
 * These are operations for working with an atomic counter type.
 *
 * A quick refresher on memory barriers: "release" semantics prevent
 * reordering with respect to previous operations, and "acquire"
 * semantics prevent reordering with respect to following operations.
 */

/*
  During 2012, both Clang (3.1) and GCC (4.7) introduced new atomic
  builtins.  These were introduced to implement the atomics in the C11
  and C++11 standards, and they are the most preferred
  implementations.

  Yet <stdatomic.h> is nowhere to be found on my system, yet.

  If the 2012 builtins are unavailable, we use inline assembly to
  avoid the unnecessary full barriers provided by the __sync builtins.
  On unknown platforms, we then fall back to the __sync builtins.
*/

/**
 * @brief Atomic counter type.
 *
 * Guaranteed to be at least as wide as int.
 */
typedef struct pce_atomic_s pce_atomic_t;

/**
 * @brief Set an atomic counter.
 */
PCE_INLINE
void
pce_atomic_set(pce_atomic_t *p, int x);

/**
 * @brief Set an atomic counter with release semantics.
 */
PCE_INLINE
void
pce_atomic_set_release(pce_atomic_t *p, int x);

/**
 * @brief Get an atomic counter value.
 */
PCE_INLINE
int
pce_atomic_get(pce_atomic_t *p);

/**
 * @brief Get an atomic counter value with acquire semantics.
 */
PCE_INLINE
int
pce_atomic_get_acquire(pce_atomic_t *p);

/**
 * @brief Increment an atomic counter value.
 */
PCE_INLINE
void
pce_atomic_inc(pce_atomic_t *p);

/**
 * @brief Decrement an atomic counter value.
 */
PCE_INLINE
void
pce_atomic_dec(pce_atomic_t *p);

/**
 * @brief Add a number to an atomic counter, returning the previous
 * value.
 */
PCE_INLINE
int
pce_atomic_fetch_add(pce_atomic_t *p, int x);

/**
 * @brief Add a number to an atomic counter, returning the previous
 * value, with acquire semantics.
 */
PCE_INLINE
int
pce_atomic_fetch_add_acquire(pce_atomic_t *p, int x);

/**
 * @brief Add a number to an atomic counter, returning the previous
 * value, with release semantics.
 */
PCE_INLINE
int
pce_atomic_fetch_add_release(pce_atomic_t *p, int x);

/**
 * @brief Add a number to an atomic counter, returning the previous
 * value, with acquire and release semantics.
 */
PCE_INLINE
int
pce_atomic_fetch_add_acq_rel(pce_atomic_t *p, int x);

#if !defined(PCE_HAS_ATOMIC) && defined(__has_feature)
#if __has_feature(c_atomic)
#define PCE_HAS_ATOMIC 1

/*
  ============================================================
  Clang atomics

  Available starting in Clang 3.1.  We detect Clang first because
  Clang impersonates GCC.
*/

struct pce_atomic_s {
    _Atomic(int) v;
};

PCE_INLINE
void
pce_atomic_set(pce_atomic_t *p, int x)
{
    __c11_atomic_store_n(&p->v, x, __ATOMIC_RELAXED);
}

PCE_INLINE
void
pce_atomic_set_release(pce_atomic_t *p, int x)
{
    __c11_atomic_store_n(&p->v, x, __ATOMIC_RELEASE);
}

PCE_INLINE
int
pce_atomic_get(pce_atomic_t *p)
{
    return __c11_atomic_load_n(&p->v, __ATOMIC_RELAXED);
}

PCE_INLINE
int
pce_atomic_get_acquire(pce_atomic_t *p)
{
    return __c11_atomic_load_n(&p->v, __ATOMIC_ACQUIRE);
}

PCE_INLINE
void
pce_atomic_inc(pce_atomic_t *p)
{
    __c11_atomic_fetch_add(&p->v, 1, __ATOMIC_RELAXED);
}

PCE_INLINE
void
pce_atomic_dec(pce_atomic_t *p)
{
    __c11_atomic_fetch_add(&p->v, -1, __ATOMIC_RELAXED);
}

PCE_INLINE
int
pce_atomic_fetch_add(pce_atomic_t *p, int x)
{
    return __c11_atomic_fetch_add(&p->v, x, __ATOMIC_RELAXED);
}

PCE_INLINE
int
pce_atomic_fetch_add_acquire(pce_atomic_t *p, int x)
{
    return __c11_atomic_fetch_add(&p->v, x, __ATOMIC_ACQUIRE);
}

PCE_INLINE
int
pce_atomic_fetch_add_release(pce_atomic_t *p, int x)
{
    return __c11_atomic_fetch_add(&p->v, x, __ATOMIC_RELEASE);
}

PCE_INLINE
int
pce_atomic_fetch_add_acq_rel(pce_atomic_t *p, int x)
{
    return __c11_atomic_fetch_add(&p->v, x, __ATOMIC_ACQ_REL);
}

#endif
#endif

#if !defined(PCE_HAS_ATOMIC) && \
    __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 7)
#define PCE_HAS_ATOMIC 1

/*
  ============================================================
  GCC atomics

  Available in GCC 4.7.
*/

struct pce_atomic_s {
    int v;
};

PCE_INLINE
void
pce_atomic_set(pce_atomic_t *p, int x)
{
    __atomic_store_n(&p->v, x, __ATOMIC_RELAXED);
}

PCE_INLINE
void
pce_atomic_set_release(pce_atomic_t *p, int x)
{
    __atomic_store_n(&p->v, x, __ATOMIC_RELEASE);
}

PCE_INLINE
int
pce_atomic_get(pce_atomic_t *p)
{
    return __atomic_load_n(&p->v, __ATOMIC_RELAXED);
}

PCE_INLINE
int
pce_atomic_get_acquire(pce_atomic_t *p)
{
    return __atomic_load_n(&p->v, __ATOMIC_ACQUIRE);
}

PCE_INLINE
void
pce_atomic_inc(pce_atomic_t *p)
{
    __atomic_fetch_add(&p->v, 1, __ATOMIC_RELAXED);
}

PCE_INLINE
void
pce_atomic_dec(pce_atomic_t *p)
{
    __atomic_fetch_add(&p->v, -1, __ATOMIC_RELAXED);
}

PCE_INLINE
int
pce_atomic_fetch_add(pce_atomic_t *p, int x)
{
    return __atomic_fetch_add(&p->v, x, __ATOMIC_RELAXED);
}

PCE_INLINE
int
pce_atomic_fetch_add_acquire(pce_atomic_t *p, int x)
{
    return __atomic_fetch_add(&p->v, x, __ATOMIC_ACQUIRE);
}

PCE_INLINE
int
pce_atomic_fetch_add_release(pce_atomic_t *p, int x)
{
    return __atomic_fetch_add(&p->v, x, __ATOMIC_RELEASE);
}

PCE_INLINE
int
pce_atomic_fetch_add_acq_rel(pce_atomic_t *p, int x)
{
    return __atomic_fetch_add(&p->v, x, __ATOMIC_ACQ_REL);
}

#endif

#if !defined(PCE_HAS_ATOMIC) && defined(__GNUC__) && defined(PCE_CPU_X86)
#define PCE_HAS_ATOMIC 1

/*
  ============================================================
  x86 Inline Assembly
*/

struct pce_atomic_s {
    volatile int v;
};

PCE_INLINE
void
pce_atomic_set(pce_atomic_t *p, int x)
{
    p->v = x;
}

#define pce_atomic_set_release pce_atomic_set

PCE_INLINE
int
pce_atomic_get(pce_atomic_t *p)
{
    return p->v;
}

#define pce_atomic_get_acquire pce_atomic_get

PCE_INLINE
void
pce_atomic_inc(pce_atomic_t *p)
{
    __asm__ __volatile__(
        "lock; incl %0"
        : "+m"(p->v));
}

PCE_INLINE
void
pce_atomic_dec(pce_atomic_t *p)
{
    __asm__ __volatile__(
        "lock; decl %0"
        : "+m"(p->v));
}

PCE_INLINE
int
pce_atomic_fetch_add_acq_rel(pce_atomic_t *p, int x)
{
    int r;
    __asm__ __volatile__(
        "lock; xaddl %0, %1"
        : "=r"(r), "=m"(*p)
        : "0"(x), "m"(*p)
        : "memory");
    return r;
}

#define pce_atomic_fetch_add_acquire pce_atomic_fetch_add_acq_rel
#define pce_atomic_fetch_add_release pce_atomic_fetch_add_acq_rel
#define pce_atomic_fetch_add pce_atomic_fetch_add_acq_rel

#endif

#if !defined(PCE_HAS_ATOMIC) && defined(__GNUC__) && defined(PCE_CPU_PPC)
#define PCE_HAS_ATOMIC 1

/*
  ============================================================
  PowerPC Inline Assembly
*/

struct pce_atomic_s {
    volatile int v;
};

PCE_INLINE
void
pce_atomic_lwsync(void)
{
    __asm__ __volatile__("lwsync" : : : "memory");    
}

PCE_INLINE
void
pce_atomic_set(pce_atomic_t *p, int x)
{
    p->v = x;
}

PCE_INLINE
void
pce_atomic_set_release(pce_atomic_t *p, int x)
{
    pce_atomic_lwsync();
    p->v = x;
}

PCE_INLINE
int
pce_atomic_get(pce_atomic_t *p)
{
    return p->v;
}

PCE_INLINE
int
pce_atomic_get_acquire(pce_atomic_t *p)
{
    int r = p->v;
    pce_atomic_lwsync();
    return r;
}

PCE_INLINE
int
pce_atomic_fetch_add(pce_atomic_t *p, int x)
{
    int r, t;
    __asm__ __volatile__(
        "1:  lwarx  %0,0,%2\n"
        "    add    %1,%0,%3\n"
        "    stwcx. %1,0,%2\n"
        "    bne    1b"
        : "=&r"(r), "=&r"(t)
        : "r"(&p->v), "r"(x)
        : "cc", "xer");
    (void) t;
    return r;
}

PCE_INLINE
void
pce_atomic_inc(pce_atomic_t *p)
{
    pce_atomic_fetch_add(p, 1);
}

PCE_INLINE
void
pce_atomic_dec(pce_atomic_t *p)
{
    pce_atomic_fetch_add(p, -1);
}

PCE_INLINE
int
pce_atomic_fetch_add_acquire(pce_atomic_t *p, int x)
{
    int r;
    r = pce_atomic_fetch_add(p, x);
    pce_atomic_lwsync();
    return r;
}

PCE_INLINE
int
pce_atomic_fetch_add_release(pce_atomic_t *p, int x)
{
    int r;
    pce_atomic_lwsync();
    r = pce_atomic_fetch_add(p, x);
    return r;
}

PCE_INLINE
int
pce_atomic_fetch_add_acq_rel(pce_atomic_t *p, int x)
{
    int r;
    pce_atomic_lwsync();
    r = pce_atomic_fetch_add(p, x);
    pce_atomic_lwsync();
    return r;
}

#endif

#if !defined(PCE_HAS_ATOMIC) && \
    (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 1))
#define PCE_HAS_ATOMIC 1

/*
  ============================================================
  GCC atomic builtins
*/

struct pce_atomic_s {
    volatile int v;
};

PCE_INLINE
void
pce_atomic_set(pce_atomic_t *p, int x)
{
    p->v = x;
}

PCE_INLINE
void
pce_atomic_set_release(pce_atomic_t *p, int x)
{
    __sync_synchronize();
    p->v = x;
}

PCE_INLINE
int
pce_atomic_get(pce_atomic_t *p)
{
    return p->v;
}

PCE_INLINE
int
pce_atomic_get_acquire(pce_atomic_t *p)
{
    int r = p->v;
    __sync_synchronize();
    return r;
}

PCE_INLINE
void
pce_atomic_inc(pce_atomic_t *p)
{
    __sync_fetch_and_add(&p->v, 1);
}

PCE_INLINE
void
pce_atomic_dec(pce_atomic_t *p)
{
    __sync_fetch_and_add(&p->v, -1);
}

PCE_INLINE
int
pce_atomic_fetch_add_acq_rel(pce_atomic_t *p, int x)
{
    return __sync_fetch_and_add(&p->v, x);
}

#define pce_atomic_fetch_add_acquire pce_atomic_fetch_add_acq_rel
#define pce_atomic_fetch_add_release pce_atomic_fetch_add_acq_rel
#define pce_atomic_fetch_add pce_atomic_fetch_add_acq_rel

#endif

#if !defined(PCE_HAS_ATOMIC) && defined(_MSC_VER)
#define PCE_HAS_ATOMIC 1
#include <intrin.h>

/*
  ============================================================
  Visual C++ Intrinsics
*/

struct pce_atomic_s {
    volatile long v;
};

PCE_INLINE
void
pce_atomic_set(pce_atomic_t *p, int x)
{
    p->v = x;
}

PCE_INLINE
void
pce_atomic_set_release(pce_atomic_t *p, int x)
{
    _WriteBarrier();
    p->v = x;
}

PCE_INLINE
int
pce_atomic_get(pce_atomic_t *p)
{
    return p->v;
}

PCE_INLINE
int
pce_atomic_get_acquire(pce_atomic_t *p)
{
    int r = (int) p->v;
    _ReadBarrier();
    return r;
}

PCE_INLINE
void
pce_atomic_inc(pce_atomic_t *p)
{
    _InterlockedIncrement(&p->v);
}

PCE_INLINE
void
pce_atomic_dec(pce_atomic_t *p)
{
    _InterlockedDecrement(&p->v);
}

PCE_INLINE
int
pce_atomic_fetch_add_acq_rel(pce_atomic_t *p, int x)
{
    return (int) _InterlockedExchangeAdd(&p->v, x);
}

/* Other intrinsics not available on x86 or x64.  */
#define pce_atomic_fetch_add_acquire pce_atomic_fetch_add_acq_rel
#define pce_atomic_fetch_add_release pce_atomic_fetch_add_acq_rel
#define pce_atomic_fetch_add pce_atomic_fetch_add_acq_rel

#endif

#if !defined(PCE_HAS_ATOMIC)
#error "No support for atomics found"
#endif

#ifdef __cplusplus
}
#endif
#endif
