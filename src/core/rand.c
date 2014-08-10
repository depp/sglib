/* Copyright 2012-2013 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "sg/rand.h"
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

struct sg_rand_state sg_rand_global;

static int
sg_rand_getentropy(unsigned *array, unsigned count);

/* FIXME: Vet this PRNG.  */

void
sg_rand_seed(struct sg_rand_state *array, unsigned count)
{
    unsigned *p, x, i;
    time_t t;
    int r;

    if (!count)
        return;

    p = malloc(sizeof(unsigned) * count * 3);
    if (!p)
        goto fallback;
    r = sg_rand_getentropy(p, count * 3);
    if (!r) {
        free(p);
        goto fallback;
    }
    for (i = 0; i < count; ++i) {
        array[i].x0 = p[i*3+0];
        array[i].x1 = p[i*3+1];
        array[i].c  = p[i*3+2];
    }
    free(p);
    return;

fallback:
    /* FIXME: emit warning? */
    t = time(NULL);
    x = (unsigned) t;
    for (i = 0; i < count; ++i) {
        array[i].x0 = x;
        array[i].x1 = 0x038acaf3U;
        array[i].c = 0xa2cc5886U;
        x += 0xd559fc9aU;
    }
}

unsigned
sg_irand(struct sg_rand_state *s)
{
    unsigned A = 4284966893U;
    uint64_t y;
    y = (uint64_t) s->x0 * A + s->c;
    s->x0 = s->x1;
    s->x1 = (unsigned) y;
    s->c = y >> 32;
    return (unsigned) y;
}

unsigned
sg_girand(void)
{
    return sg_irand(&sg_rand_global);
}

double
sg_frand(struct sg_rand_state *s)
{
    return (float) sg_irand(s) * (1.0 / 4294967296.0);
}

double
sg_gfrand(void)
{
    return sg_frand(&sg_rand_global);
}

#if defined(_WIN32)

static int
sg_rand_getentropy(unsigned *array, unsigned count)
{
    return -1;
}

#else

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

static int
sg_rand_getentropy(unsigned *array, unsigned count)
{
    unsigned char *p = (unsigned char *) array;
    size_t rem = count * sizeof(unsigned);
    int fdes, e;
    ssize_t amt;

    fdes = open("/dev/urandom", O_RDONLY);
    if (fdes < 0) {
        e = errno;
        if (e != ENOENT)
            return -1;
        fdes = open("/dev/random", O_RDONLY);
        if (fdes < 0)
            return -1;
    }

    while (rem) {
        amt = read(fdes, p, rem);
        if (amt > 0) {
            rem -= amt;
            p += amt;
        } else if (amt == 0) {
            break;
        } else if (amt < 0) {
            break;
        }
    }

    close(fdes);

    return rem == 0 ? 0 : -1;
}

#endif
