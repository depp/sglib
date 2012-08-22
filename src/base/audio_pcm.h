#ifndef BASE_AUDIO_PCM_H
#define BASE_AUDIO_PCM_H
#include "defs.h"
#ifdef __cplusplus
extern "C" {
#endif

struct sg_audio_pcm {
    /* Number of audio frames (each frame is a sequence of samples) */
    int nframe;
    /* Number of channels (1 or 2) */
    int nchan;
    /* Sample rate, in Hz */
    int rate;
    /* Sample data, interleaved channels.  There is always one extra
       frame past the end, containing zero data.  */
    float *data;
};

void
sg_audio_pcm_init(struct sg_audio_pcm *pcm);

/* Allocate the data field in a PCM structure which is completely
   initialized.  Returns 0 if successful, or nonzero for memory
   error.  */
int
sg_audio_pcm_alloc(struct sg_audio_pcm *pcm);

void
sg_audio_pcm_destroy(struct sg_audio_pcm *pcm);

void
sg_audio_copy_float32(void *dest, const void *src,
                      unsigned nelem, int swap);

/*
  Resample PCM audio data.  The target buffer is has 'buflen' stereo
  frammes and runs at the given sample rate.  The source PCM will be
  written starting at the sample given by 'offset', which is measured
  in frames in the target buffer.

  For example, a rate of 48000 and an offset of 24000 means that the
  PCM will be written starting 500 ms into the buffer.
*/
void
sg_audio_pcm_resample(struct sg_audio_pcm *pcm, float *buf, int buflen,
                      int offset, int rate);

#ifdef __cplusplus
}
#endif
#endif
