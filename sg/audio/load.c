#include "sg/audio_pcm.h"
#include "sg/error.h"
#include <string.h>

int
sg_audio_pcm_load(struct sg_audio_pcm *buf, const void *data, size_t len,
                  struct sg_error **err)
{
    const unsigned char *p = data;
    if (len >= 12 && !memcmp(p, "RIFF", 4) && !memcmp(p + 8, "WAVE", 4))
        return sg_audio_pcm_loadwav(buf, data, len, err);
    sg_error_data(err, "audio");
    return -1;
}
