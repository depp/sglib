#include "audio_pcm.h"
#include <stdlib.h>

#include <stdio.h>

void
sg_audio_pcm_init(struct sg_audio_pcm *pcm)
{
    pcm->nframe = 0;
    pcm->nchan = 0;
    pcm->rate = 0;
    pcm->data = NULL;
}

int
sg_audio_pcm_alloc(struct sg_audio_pcm *pcm)
{
    size_t sz;
    float *data;
    int i;
    sz = sizeof(float) * (pcm->nframe + 1) * pcm->nchan;
    data = malloc(sz);
    if (!data)
        return -1;
    for (i = 0; i < pcm->nchan; ++i)
        data[pcm->nframe * pcm->nchan + i] = 0.0f;
    pcm->data = data;
    return 0;
}

void
sg_audio_pcm_destroy(struct sg_audio_pcm *pcm)
{
    free(pcm->data);
}

void
sg_audio_pcm_resample(struct sg_audio_pcm *pcm, float *buf, int buflen,
                      int offset, int rate)
{
    const float *restrict ip;
    float *restrict op;
    double rr;
    int i, pos, plen;
    float x;

    rr = (double) pcm->rate / rate;
    // printf("ratio: %f\n", rr);
    ip = pcm->data;
    op = buf;
    plen = pcm->nframe;
    switch (pcm->nchan) {
    case 1:
        for (i = 0; i < buflen; ++i) {
            pos = (int) (rr * (i - offset));
            if (pos < 0 || pos >= plen)
                x = 0.0f;
            else
                x = ip[pos];
            op[2*i+0] = x;
            op[2*i+1] = x;
        }
        break;

    default:
        puts("sg_audio_pcm_resample: not supported\n");
        break;
    }
}
