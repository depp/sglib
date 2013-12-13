/* Copyright 2013 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "defs.h"
#include "sg/log.h"
#include "sg/audio_buffer.h"
#include "sg/audio_file.h"
#include "sg/file.h"
#include "sg/log.h"
#include <stdlib.h>
#include <string.h>

void
load_audio(struct sg_audio_buffer *buffer, const char *path)
{
    struct sg_buffer *buf;
    struct sg_audio_buffer *pcm;
    size_t pcmcount;
    int r;

    buf = sg_file_get(path, strlen(path), SG_RDONLY,
                      SG_AUDIO_FILE_EXTENSIONS,
                      1024 * 1024 * 64, NULL);
    if (!buf)
        abort();

    r = sg_audio_file_load(&pcm, &pcmcount, buf->data, buf->length, NULL);
    if (r)
        abort();
    if (pcmcount != 1) {
        sg_logf(sg_logger_get(NULL), SG_LOG_ERROR,
                "Ogg file has multiple sections: %s", path);
        abort();
    }

    r = sg_audio_buffer_convert(&pcm[0], SG_AUDIO_S16NE, NULL);
    if (r)
        abort();

    memcpy(buffer, &pcm[0], sizeof(struct sg_audio_buffer));
}
