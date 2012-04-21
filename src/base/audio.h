#ifndef BASE_AUDIO_H
#define BASE_AUDIO_H
#include "resource.h"
#ifdef __cplusplus
extern "C" {
#endif
struct sg_error;

/* Flags for sg_audio_source */
enum {
    SG_AUDIO_TERMINATE = 1u << 0
};

/* This should retain references to any necessary resources.  */
struct sg_audio_source {
    int flags;
    /* Fill the buffer with the given number of frames.  Returns the
       number of frames actually filled with data, starting with the
       beginning of the buffer.  If 0 is returned, then silence is
       played.  */
    int (*fill)(struct sg_audio_source *sp, float *buf, int frames);
    /* Free the audio source.  */
    void (*free)(struct sg_audio_source *sp);
};

/* The rate is the audio system sample rate, in Hz.  This function may
   be called multiple times if there are, e.g., two playback systems
   (this would be the case when recording audio to disk: one stream
   gets played, the other stream gets written to disk).

   All communication about the object (e.g., when to fade out) happens
   without help from the audio system, usually by setting flags in the
   'obj' object.  */
typedef struct sg_audio_source *
(*sg_audio_source_f)(void *obj, int rate, struct sg_error **err);

/* Play the given source at the given time.  Return 0 for success,
   nonzero for failure.  */
int
sg_audio_play(unsigned time, void *obj, sg_audio_source_f func,
              struct sg_error **err);

/* A resource for audio files (WAV, OGG, etc.) */
struct sg_audio_file;

/* Create an audio file resource from the specified path.  May return
   a previously created audio file object.  */
struct sg_audio_file *
sg_audio_file_new(const char *path, struct sg_error **err);

void
sg_audio_file_incref(struct sg_audio_file *fp);

void
sg_audio_file_decref(struct sg_audio_file *fp);

/* Play the given file at the given time.  Return 0 for success or
   nonzero for failure.  */
int
sg_audio_file_play(struct sg_audio_file *fp, unsigned time,
                   struct sg_error **err);

/**********************************************************************/

/* An audio system can have mixed audio pulled from it.  All sounds
   played will be broadcasted to all audio systems.  */
struct sg_audio_system;

/* Create a new audio system that produces audio at the given sample
   rate.  */
struct sg_audio_system *
sg_audio_system_new(int rate, struct sg_error **err);

void
sg_audio_system_free(struct sg_audio_system *sp);

/* Pull the given number of frames of interleaved stereo from the
   audio system.  */
void
sg_audio_system_pull(struct sg_audio_system *sp,
                     unsigned time, float *buf, int frames);

/**********************************************************************/

/* Init the main audio output for this platform.  */
void
sg_audio_initsys(void);

#ifdef __cplusplus
}
#endif
#endif
