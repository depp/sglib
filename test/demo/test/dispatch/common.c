/* Copyright 2013 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "dispatch_impl.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ITERCOUNT 60000
#define NOISY 0

static const int ORDER[] = {
    2, 5, 9, 8, 1, 4, 3, 6, 0, 7,
    11, 17, 10, 16, 14, 12, 19, 15, 18, 13,
    21, 28, 26, 22, 25, 29, 27, 20, 24, 23
};

#define fail() fail_func(__LINE__)

static void
fail_func(int line)
{
    fprintf(stderr, "FAILED: line %d\n", line);
    exit(1);
}

static void
pop(struct sg_dispatch_queue *q, int i)
{
    struct sg_dispatch_task t;
    int p;
    sg_dispatch_queue_pop(q, &t);
    p = - ((int) (t.priority >> 16) - 0x8000);
    if (p != i) {
        fprintf(stderr, "Priority: expected %d, got %d\n", i, p);
        exit(1);
    }
    if (t.cxt != (void *) (intptr_t) i) {
        fprintf(stderr, "Context: expected %p, got %p\n",
                (void *) (intptr_t) i, t.cxt);
        exit(1);
    }
    if (t.func != (void (*)(void *)) (intptr_t) i) {
        fprintf(stderr, "Function: expected %p, got %p\n",
                (void *) (intptr_t) i, (void *) t.func);
        fail();
    }
}

/* 0-1, 1-3, 3-7, 7-15
   1-2 2-4*/

static void
dump(struct sg_dispatch_queue *q)
{
    struct sg_dispatch_task *t;
    unsigned n = q->count, i, j, k, ll;
    int p, p1, p2, badheap = 0;
    if (!n) {
        if (NOISY)
            puts("empty");
        return;
    }
    t = q->tasks;
    if (NOISY)
        printf("HEAP: (%d)\n", (int) n);
    i = 0;
    for (j = 0; j < 5 && i < n; ++j) {
        ll = 3 * (1 << (4 - j));
        for (i = (1 << j) - 1; i < (2 << j) - 1 && i < n; ++i) {
            p = -((int) (t[i].priority >> 16) - 0x8000);
            if (i * 2 + 1 < n)
                p1 = -((int) (t[i*2+1].priority >> 16) - 0x8000);
            else
                p1 = p + 1;
            if (i * 2 + 2 < n)
                p2 = -((int) (t[i*2+2].priority >> 16) - 0x8000);
            else
                p2 = p + 1;
            if (p1 < p || p2 < p)
                badheap = 1;
            if (p < 0 || p >= 30) {
                putchar('\n');
                fprintf(stderr, "Bad prio: %d\n", p);
                exit(1);
            }
            if (t[i].cxt != (void *) (intptr_t) p) {
                putchar('\n');
                fprintf(stderr, "Bad context\n");
                exit(1);
            }
            if (t[i].func != (void (*)(void *)) (intptr_t) p) {
                putchar('\n');
                fprintf(stderr, "Bad func\n");
                exit(1);
            }
            if (NOISY) {
                printf(" %02d", p);
                for (k = 3; k < ll; ++k)
                    putchar(' ');
            }
        }
        if (NOISY)
            putchar('\n');
    }
    if (badheap) {
        fprintf(stderr, "Bad heap\n");
        exit(1);
    }
}

int
main(int argc, char *argv[])
{
    int i, j, k;
    struct sg_dispatch_queue q;
    (void) argc;
    (void) argv;
    memset(&q, 0, sizeof(q));
    for (i = 0; i < ITERCOUNT; ++i) {
        j = 0;
        k = 0;
        for (; j < 15; ++j) {
            if (NOISY)
                printf("insert %d\n", j);
            if (q.count != (unsigned) j)
                fail();
            sg_dispatch_queue_push(
                &q, -ORDER[j],
                (void *) (intptr_t) ORDER[j],
                (void (*)(void *)) (intptr_t) ORDER[j]);
            dump(&q);
        }
        for (; k < 10; ++k) {
            if (NOISY)
                printf("pop %d\n", k);
            if (q.count != (unsigned) (j - k))
                fail();
            pop(&q, k);
            dump(&q);
        }
        for (; j < 30; ++j) {
            if (NOISY)
                printf("insert %d\n", j);
            if (q.count != (unsigned) (j - k))
                fail();
            sg_dispatch_queue_push(
                &q, -ORDER[j],
                (void *) (intptr_t) ORDER[j],
                (void (*)(void *)) (intptr_t) ORDER[j]);
            dump(&q);
        }
        for (; k < 30; ++k) {
            if (NOISY)
                printf("pop %d\n", k);
            if (q.count != (unsigned) (j - k))
                fail();
            pop(&q, k);
            dump(&q);
        }
        if (q.count)
            fail();
    }
    return 0;
}
