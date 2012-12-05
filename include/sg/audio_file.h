/* Copyright 2012 Dietrich Epp <depp@zdome.net> */
#ifndef SG_AUDIO_FILE_H
#define SG_AUDIO_FILE_H
#include "libpce/thread.h"
#include "sg/resource.h"
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

/* Maximum audio file length, in seconds.  This is 30 minutes, which
   is actually kind of ridiculous.  */
#define SG_AUDIO_MAXLENGTH (30*60)

enum {
    /* The file is loaded, and the sample rate is the same as the
       audio system's current sample rate.  This flag will only be
       cleared when the system sample rate changes, which only happens
       on the main thread when no mixdowns exist.  So if this flag is
       set, a mixdown can rely on it continuing to be set as long as
       the mixdown exists.  */
    SG_AUDIOFILE_LOADED = 1u << 0,

    /* Frames are interleaved stereo, otherwise the frames are
       mono.  */
    SG_AUDIOFILE_STEREO = 1u << 1,

    /* The sample rate is not the original sample rate of the file, it
       has ben converted.  */
    SG_AUDIOFILE_RESAMPLED = 1u << 2,
};

/* THIS DOESN'T WORK */
struct sg_audio_file {
    struct sg_resource r;
    const char *path;

    /* FIXME: FIXMEATOMIC: make this field atomic */
    /* Audio file flags -- this sholud be written with 'release'
       semantics and loaded with 'acquire' semantics.  */
    unsigned flags;

    /* Number of frames in audio file */
    int nframe;

    /* Length of file, in msec (ticks) */
    int playtime;

    /* Sample rate, in Hz */
    int rate;

    /* Sample data (16-bit samples, native endian) */
    short *data;
};

/* Create an audio file resource from the specified path.  May return
   a previously created audio file object.  */
struct sg_audio_file *
sg_audio_file_new(const char *path, struct sg_error **err);

#ifdef __cplusplus
}
#endif
#endif
