#include "audio_pcm.h"
#include <string.h>

void
sg_audio_copy_float32(void *dest, const void *src,
                      unsigned nelem, int swap)
{
    unsigned char *dp;
    const unsigned char *sp;
    unsigned i;
    if (!swap) {
        memcpy(dest, src, (size_t) nelem * 4);
    } else {
        dp = dest;
        sp = src;
        for (i = 0; i < nelem; ++i) {
            dp[0] = sp[3];
            dp[1] = sp[2];
            dp[2] = sp[1];
            dp[3] = sp[0];
            dp += 4;
            sp += 4;
        }
    }
}
