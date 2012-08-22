#ifndef BASE_AUDIO_FILE_H
#define BASE_AUDIO_FILE_H
#include "audio_pcm.h"
#include "resource.h"
#include "thread.h"
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
struct sg_error;
struct sg_audio_pcm;

/* Minimum and maximum supported sample rates (in Hz) for audio
   files.  */
#define SG_AUDIO_MINRATE 8000
#define SG_AUDIO_MAXRATE 96000

typedef enum {
    /* File is not loaded yet */
    SG_AUDIO_UNLOADED,
    /* Raw PCM data (wav, aiff, etc.) */
    SG_AUDIO_PCM
} sg_audio_file_type_t;

struct sg_audio_file {
    struct sg_resource r;
    const char *path;
    /* Everything below here has mutex.  */
    struct sg_lock lock;
    sg_audio_file_type_t type;
    union {
        struct sg_audio_pcm pcm;
    } data;
};

/* Create an audio file resource from the specified path.  May return
   a previously created audio file object.  */
struct sg_audio_file *
sg_audio_file_new(const char *path, struct sg_error **err);

#ifdef __cplusplus
}
#endif
#endif
