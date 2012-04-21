#ifndef BASE_AUDIO_UTIL_H
#define BASE_AUDIO_UTIL_H
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

int
sg_audio_pcm_fill(struct sg_audio_pcm *pcm, float *buf, int nframes,
                  int rate, int pos);

/* Copy float32 data to float32 data.  If swap is true, this will swap
   the byte order in the copy.  */
void
sg_audio_copy_float32(void *dest, const void *src,
                      unsigned nelem, int swap);

#ifdef __cplusplus
}
#endif
#endif
