/* Implementation of Austin Appleby's "Murmur Hash"
   See: http://sites.google.com/site/murmurhash/
   Note that results are different by platform.  This is fine.  */
#include "hash.h"

unsigned
murmur_hash(void const *data, size_t len, unsigned seed)
{
    const unsigned char *p = data;
    unsigned m = 0x5bd1e995, h = seed ^ len, k;
    int r = 24;
    while (len >= 4) {
        k = *(unsigned *)data;
        k *= m; 
        k ^= k >> r; 
        k *= m; 
        h *= m; 
        h ^= k;
        p += 4;
        len -= 4;
    }

    switch(len) {
    case 3: h ^= p[2] << 16;
    case 2: h ^= p[1] << 8;
    case 1: h ^= p[0];
            h *= m;
    };

    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;

    return h;
} 
