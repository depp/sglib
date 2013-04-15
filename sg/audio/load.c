#include "config.h"
#include "sg/audio_pcm.h"
#include "sg/error.h"
#include <stdlib.h>
#include <string.h>

int
sg_audio_pcm_load(struct sg_audio_pcm **buf, size_t *bufcount,
                  const void *data, size_t len,
                  struct sg_error **err)
{
    const unsigned char *p = data;
    struct sg_audio_pcm *bufp;
    int r;

    if (len >= 12 && !memcmp(p, "RIFF", 4) && !memcmp(p + 8, "WAVE", 4)) {
        bufp = malloc(sizeof(*bufp));
        if (!bufp) {
            sg_error_nomem(err);
            return -1;
        }
        r = sg_audio_pcm_loadwav(bufp, data, len, err);
        if (r) {
            free(bufp);
            return -1;
        }
        *buf = bufp;
        *bufcount = 1;
        return 0;
    }

#if defined(ENABLE_OGG)
    if (len >= 4 && !memcmp(p, "OggS", 4))
        return sg_audio_pcm_loadogg(buf, bufcount, data, len, err);
#endif

    sg_error_data(err, "audio");
    return -1;
}
