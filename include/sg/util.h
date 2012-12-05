#ifndef BASE_UTIL_H
#define BASE_UTIL_H
#include "defs.h"
#include <stddef.h>

SG_INLINE size_t
sg_align(size_t x)
{
    return (x + (sizeof(void *) - 1)) & -sizeof(void *);
}

SG_INLINE unsigned
sg_round_up_pow2(unsigned x)
{
    unsigned a = x - 1;
    a |= a >> 1;
    a |= a >> 2;
    a |= a >> 4;
    a |= a >> 8;
    a |= a >> 16;
    return a + 1;
}

#endif
