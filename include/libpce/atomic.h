/* Copyright 2012 Dietrich Epp <depp@zdome.net> */
#ifndef PCE_ATOMIC_H
#define PCE_ATOMIC_H
#include "libpce/arch.h"
#include "libpce/attribute.h"
#ifdef __cplusplus
extern "C" {
#endif

/*
  During 2012, both Clang (3.1) and GCC (4.7) introduced new atomic
  builtins.  These were introduced to implement the atomics in the C11
  and C++11 standards, and they are the most preferred
  implementations.

  Yet <stdatomic.h> is nowhere to be found on my system, yet.

  If the 2012 builtins are unavailable, we use inline assembly to
  avoid the unnecessary full barriers provided by the __sync builtins.
  On unknown platforms, we then fall back to the __sync builtins.

  pce_atomic_t -- atomic integer type (int)

  pce_atomic_set(p, x) -- set an atomic: { *p = x; }

  pce_atomic_get(p) -- get the value of an atomic { return *p; }

  pce_atomic_inc(p) -- increment { *p++; }

  pce_atomic_dec(p) -- decrement { *p--; }

  pce_atomic_fetch_add(p, x) -- addition, returning the old value
      { int tmp = *p; *p = tmp + x; return tmp; }

  Plus barrier versions of atomic_fetch_add -- acquire and release
  barriers are available, as well as an acquire + release barrier.
  Barrier versions of set and get are also available, but only
  set_release and get_acquire.
*/

typedef struct pce_atomic_s pce_atomic_t;

#if !defined(PCE_HAS_ATOMIC) && defined(__has_feature)
#if __has_feature(c_atomic)
#define PCE_HAS_ATOMIC 1

/*
  Clang atomics starting in Clang 3.1.  We detect Clang first because
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
  GCC 4.7 provides an atomic interface for implementing C11 and C++11
  atomics.
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
  If we still have GCC's inline assembly available, we use that.
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
  As a last resort, we use the old GCC atomic builtins.
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

PCE_INLINE
int
pce_atomic_fetch_add_acquire(pce_atomic_t *p, int x)
{
    return (int) _InterlockedExchangeAdd_acq(&p->v, x);
}

PCE_INLINE
int
pce_atomic_fetch_add_release(pce_atomic_t *p, int x)
{
    return (int) _InterlockedExchangeAdd_rel(&p->v, x);
}

/*
  This is a rather arbitrary choice, since there isn't a "no barrier"
  version provided by MSC.
*/
#define pce_atomic_fetch_add pce_atomic_fetch_add_acquire

#endif

#if !defined(PCE_HAS_ATOMIC)
#error "No support for atomics found"
#endif

#ifdef __cplusplus
}
#endif
#endif
