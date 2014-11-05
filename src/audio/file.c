/* Copyright 2013 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "config.h"
#include "sg/audio_buffer.h"
#include "sg/audio_file.h"
#include "sg/error.h"
#include <stdlib.h>
#include <string.h>

const char SG_AUDIO_FILE_EXTENSIONS[] = "wav:ogg:opus:oga";

int
sg_audio_file_load(struct sg_audio_buffer **buf, size_t *bufcount,
                   const void *data, size_t len,
                   struct sg_error **err)
{
    const unsigned char *p = data;
    struct sg_audio_buffer *bufp;
    int r;

    if (len >= 12 && !memcmp(p, "RIFF", 4) && !memcmp(p + 8, "WAVE", 4)) {
        bufp = malloc(sizeof(*bufp));
        if (!bufp) {
            sg_error_nomem(err);
            return -1;
        }
        r = sg_audio_file_loadwav(bufp, data, len, err);
        if (r) {
            free(bufp);
            return -1;
        }
        *buf = bufp;
        *bufcount = 1;
        return 0;
    }

#if defined ENABLE_OPUS || defined ENABLE_VORBIS
    if (len >= 4 && !memcmp(p, "OggS", 4))
        return sg_audio_file_loadogg(buf, bufcount, data, len, err);
#endif

    sg_error_data(err, "audio");
    return -1;
}
