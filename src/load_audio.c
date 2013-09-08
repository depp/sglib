/* Copyright 2013 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "defs.h"
#include "sg/log.h"
#include "sg/audio_pcm.h"
#include "sg/file.h"
#include <stdlib.h>
#include <string.h>

struct sg_audio_pcm_obj *
load_audio(const char *path)
{
    struct sg_buffer *buf;
    struct sg_audio_pcm *pcm;
    size_t pcmcount;
    struct sg_audio_pcm_obj *obj;
    int r;

    buf = sg_file_get(path, strlen(path), SG_RDONLY,
                      SG_AUDIO_PCM_EXTENSIONS,
                      1024 * 1024 * 64, NULL);
    if (!buf)
        abort();

    r = sg_audio_pcm_load(&pcm, &pcmcount, buf->data, buf->length, NULL);
    if (r)
        abort();
    if (pcmcount != 1)
        abort();

    r = sg_audio_pcm_convert(&pcm[0], SG_AUDIO_S16NE, NULL);
    if (r)
        abort();

    obj = sg_audio_pcm_obj_new();
    if (!obj)
        abort();
    memcpy(&obj->buf, &pcm[0], sizeof(obj->buf));

    return obj;
}
