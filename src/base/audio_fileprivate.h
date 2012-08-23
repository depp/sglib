#ifndef BASE_AUDIO_FILEPRIVATE_H
#define BASE_AUDIO_FILEPRIVATE_H
#include <stddef.h>
struct sg_error;
struct sg_audio_file;

/* Return 0 if the file is not a WAVE file.  Return 1 if the file is
   probably a wave file.  Just checks the first few bytes.  */
int
sg_audio_file_checkwav(const void *data, size_t length);

/* Load a WAVE file.  Return 0 for success, or -1 for error.  */
int
sg_audio_file_loadwav(struct sg_audio_file *fp,
                      const void *data, size_t length,
                      struct sg_error **err);

/* Resample an audio file to the given sample rate.  Return 0 for
   success, or -1 for error.  */
int
sg_audio_file_resample(struct sg_audio_file *fp, int rate,
                       struct sg_error **err);

typedef enum {
    SG_AUDIO_U8,
    SG_AUDIO_S16BE,
    SG_AUDIO_S16LE,
    SG_AUDIO_S24BE,
    SG_AUDIO_S24LE,
    SG_AUDIO_F32BE,
    SG_AUDIO_F32LE
} sg_audio_format_t;

/* Load raw sample data.  Return 0 for success, or nonzero for error.
   The "count" measures frames of audio.  */
int
sg_audio_file_loadraw(struct sg_audio_file *fp,
                      const void *data, size_t count,
                      sg_audio_format_t format, int nchan, int rate,
                      struct sg_error **err);

#endif
