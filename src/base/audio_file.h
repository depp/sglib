#ifndef BASE_AUDIO_FILE_H
#define BASE_AUDIO_FILE_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

/* Minimum and maximum supported sample rates (in Hz) for audio
   files.  */
#define SG_AUDIO_MINRATE 8000
#define SG_AUDIO_MAXRATE 96000

struct sg_error;
struct sg_audio_pcm;

/* Return 0 if the file is not a WAVE file.  Return 1 if the file is
   probably a wave file.  Just checks the first few bytes.  */
int
sg_audio_file_checkwav(const void *data, size_t length);

/* Load a WAVE file.  Return 0 for success, or -1 for error.  */
int
sg_audio_file_loadwav(struct sg_audio_pcm *pcm,
                      const void *data, size_t length,
                      struct sg_error **err);

#ifdef __cplusplus
}
#endif
#endif
