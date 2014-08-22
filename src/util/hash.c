/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "sg/hash.h"
#include "sg/util.h"

/*
  This is an implementation of Austin Appleby's MurmurHash3.  The
  chosen seed is 2pi.

  http://code.google.com/p/smhasher/wiki/MurmurHash
*/

unsigned
sg_hash(const void *data, size_t len)
{
    const unsigned c1 = 0xcc9e2d51, c2 = 0x1b873593;
    const unsigned char *p = data;
    size_t n = len / 4, i;
    unsigned x, y = 0xc90fdaa2;
    unsigned long long z;

    for (i = 0; i < n; i++) {
        x = *(unsigned *) (p + 4*i);
        x *= c1;
        x = (x << 15) | (x >> 17);
        x *= c2;
        y ^= x;
        y = (y << 13) | (y >> 19);
        y = y * 5 + 0xe6546b64;
    }

    x = 0;
    switch (len & 3) {
    case 3:
        x = p[4*n+2] << 16;
    case 2:
        x |= p[4*n+1] << 8;
    case 1:
        x |= p[4*n+0];
        x *= c1;
        x = (x << 15) | (x >> 17);
        x *= c2;
        y ^= x;
    }

    y ^= len;

    z = y;
    z *= 0xff51afd7ed558ccdull;
    z ^= z >> 33;
    z *= 0xc4ceb9fe1a85ec53ull;
    z ^= z >> 33;
    y = (unsigned) z;

    return y;
}
