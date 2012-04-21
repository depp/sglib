#include "audio_util.h"
#include <stdlib.h>
#include <math.h>

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

int
sg_audio_pcm_fill(struct sg_audio_pcm *pcm, float *buf, int nframes,
                  int rate, int pos)
{
    float rrate = (float) rate / (float) pcm->rate;
    int i, j, nframe = pcm->nframe;
    float l1, r1, l2, r2, p, jf, frac, *data = pcm->data;
    p = rrate * pos;

    switch (pcm->nchan) {
    case 1: 
        for (i = 0; i < nframes; ++i, p += rrate) {
            jf = floorf(p);
            frac = p - jf;
            j = (int) jf;
            if (j < 0 || j >= nframe)
                break;
            l1 = data[j];
            l2 = data[j + 1];
            l1 = l1 * (1.0f - frac) + l2 * frac;
            buf[i*2+0] = l1;
            buf[i*2+1] = l1;
        }
        break;

    case 2:
        for (i = 0; i < nframes; ++i, p += rrate) {
            jf = floorf(p);
            frac = p - jf;
            j = (int) jf;
            if (j < 0 || j >= nframe)
                break;
            l1 = data[j*2+0];
            r1 = data[j*2+1];
            l2 = data[j*2+2];
            r2 = data[j*2+3];
            l1 = l1 * (1.0f - frac) + l2 * frac;
            r1 = r1 * (1.0f - frac) + r2 * frac;
            buf[i*2+0] = l1;
            buf[i*2+1] = r2;
        }
        break;

    default:
        return 0;
    }

    return i;
}
