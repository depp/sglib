#ifndef BASE_AUDIO_MIXDOWN_H
#define BASE_AUDIO_MIXDOWN_H
/*
    An audio mixdown provides a mixdown of all audio sources.
    Mixdowns can be used safely from any thread.  However, the same
    audio mixdown cannot be used simultaneously from multiple threads.

    Audio mixdowns get their timing information from the timestamps
    passed to the sg_audio_mixdown_read() function.  A mixdown will
    automatically adjust audio timing to account for clock drift
    between the mixdown's nominal sample rate and the timestamps
    provided.

    All mixdowns must have the same sample rate, and there can only be
    two mixdowns.  In practice, this means that one mixdown plays audio
*/
#include "defs.h"
#ifdef __cplusplus
extern "C" {
#endif
struct sg_error;
struct sg_audio_mixdown;

/* Create a new audio mixdown at the given sample rate, with the given
   buffer size.  The sample rate is specified in Hz.  */
struct sg_audio_mixdown *
sg_audio_mixdown_new(int rate, int bufsize, struct sg_error **err);

struct sg_audio_mixdown *
sg_audio_mixdown_newoffline(unsigned mintime, int bufsize,
                            struct sg_error **err);

/* Free an audio mixdown.  */
void
sg_audio_mixdown_free(struct sg_audio_mixdown *SG_RESTRICT mp);

/* Read frames of interleaved stereo from an audio mixdown.  The
   timestamp corresponds to the beginning of the buffer.  Returns
   0 on success, or nonzero if the mixdown was terminated for any
   reason.  */
int
sg_audio_mixdown_read(struct sg_audio_mixdown *SG_RESTRICT mp,
                      unsigned time, float *buf);

/* Get the mixdown's calculated timestamp for the beginning of the
   next buffer.  */
unsigned
sg_audio_mixdown_timestamp(struct sg_audio_mixdown *SG_RESTRICT mp);

/* Get the nominal sample rate of the mixdown.  */
int
sg_audio_mixdown_samplerate(struct sg_audio_mixdown *SG_RESTRICT mp);

/* Get the buffer size of the mixdown.  */
int
sg_audio_mixdown_abufsize(struct sg_audio_mixdown *SG_RESTRICT mp);

#ifdef __cplusplus
}
#endif
#endif
