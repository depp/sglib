#ifndef BASE_UTIL_H
#define BASE_UTIL_H
#include "defs.h"
#include <stddef.h>

#if defined(SG_UTIL)
# define SG_INL SG_EXTERN_INLINE
#else
# define SG_INL SG_INLINE
#endif

SG_INL size_t
sg_align(size_t x);

SG_INL unsigned
sg_round_up_pow2(unsigned x);

SG_INL size_t
sg_align(size_t x)
{
    return (x + (sizeof(void *) - 1)) & -sizeof(void *);
}

SG_INL unsigned
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

#undef SG_INL

#endif
