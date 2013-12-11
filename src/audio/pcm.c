/* Copyright 2012-2013 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "sg/audio_pcm.h"
#include "sg/defs.h"
#include "sg/error.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

const char SG_AUDIO_PCM_EXTENSIONS[] = "wav:ogg:opus:oga";

const unsigned char SG_AUDIO_FORMAT_SIZE[SG_AUDIO_NFMT] = {
    1, 2, 2, 3, 3, 4, 4
};

void
sg_audio_pcm_init(struct sg_audio_pcm *buf)
{
    buf->alloc = NULL;
    buf->data = NULL;
    buf->format = SG_AUDIO_U8;
    buf->rate = 0;
    buf->nchan = 0;
    buf->nframe = 0;
}

void
sg_audio_pcm_destroy(struct sg_audio_pcm *buf)
{
    free(buf->alloc);
    buf->alloc = NULL;
    buf->data = NULL;
}

void *
sg_audio_pcm_detach(struct sg_audio_pcm *buf, struct sg_error **err)
{
    void *ptr;
    size_t sz;
    if (!buf->nframe) {
        ptr = NULL;
        free(buf->alloc);
    } else if (buf->alloc) {
        ptr = buf->alloc;
    } else if (buf->data) {
        sz = sg_audio_format_size(buf->format) * buf->nchan * buf->nframe;
        ptr = malloc(sz);
        if (!ptr) {
            sg_error_nomem(err);
            return NULL;
        }
        memcpy(ptr, buf->data, sz);
    } else {
        abort();
    }
    buf->alloc = NULL;
    buf->data = NULL;
    return ptr;
}

int
sg_audio_pcm_playtime(struct sg_audio_pcm *buf)
{
    long long msec =
        ((long long) buf->nframe * 1000 + buf->rate - 1) /
        buf->rate;
    if (msec >= INT_MAX)
        return INT_MAX;
    if (msec < 0)
        return 0;
    return (int) msec;
}

struct sg_audio_pcm_obj*
sg_audio_pcm_obj_new(void)
{
    struct sg_audio_pcm_obj *obj;
    obj = malloc(sizeof(*obj));
    if (!obj)
        return NULL;
    pce_atomic_set(&obj->refcount, 1);
    sg_audio_pcm_init(&obj->buf);
    return obj;
}

void
sg_audio_pcm_obj_free_(struct sg_audio_pcm_obj *obj)
{
    sg_audio_pcm_destroy(&obj->buf);
}
