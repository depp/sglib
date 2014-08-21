/* Copyright 2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "sound.h"
#include "sg/audio_file.h"
#include "sg/error.h"
#include "sg/file.h"
#include "sg/mixer.h"
#include <stdlib.h>
#include <string.h>

#define SG_MIXER_SOUND_MAXSZ (16 * 1024 * 1024)

struct sg_mixer_sound *
sg_mixer_sound_load(const char *path, size_t pathlen,
                    struct sg_error **err)
{
    struct sg_audio_buffer *abuf = NULL;
    size_t abufcount = 0;
    struct sg_buffer *buf = NULL;
    struct sg_mixer_sound *sp = NULL;
    int npathlen, r;
    char npath[SG_MAX_PATH], *pp;

    npathlen = sg_path_norm(npath, path, pathlen, err);
    if (npathlen < 0)
        return NULL;

    buf = sg_file_get(npath, npathlen, SG_RDONLY, SG_AUDIO_FILE_EXTENSIONS,
                      SG_MIXER_SOUND_MAXSZ, err);
    if (!buf)
        goto err;

    r = sg_audio_file_load(&abuf, &abufcount, buf->data, buf->length, err);
    if (r)
        goto err;

    /* FIXME: do something sensible here.  Do we log an error?  Do we
       allow this?  */
    if (abufcount != 1)
        abort();

    /* FIXME: log an error.  */
    if (abuf->nchan != 1 && abuf->nchan != 2)
        abort();

    r = sg_audio_buffer_convert(abuf, SG_AUDIO_S16NE, err);
    if (r)
        goto err;

    sp = malloc(sizeof(*sp) + npathlen + 1);
    if (!sp) {
        sg_error_nomem(err);
        goto err;
    }

    pp = (char *) (sp + 1);
    memcpy(pp, npath, npathlen + 1);
    pce_atomic_set(&sp->refcount, 1);
    sp->path = pp;
    sp->pathlen = npathlen;
    sp->sample.data = sg_audio_buffer_detach(abuf, err);
    if (!sp->sample.data)
        goto err;
    sp->sample.stereo = abuf->nchan == 2;
    sp->sample.length = abuf->nframe;

    sg_audio_buffer_destroy(abuf);
    sg_buffer_decref(buf);

    return sp;

err:
    if (buf)
        sg_buffer_decref(buf);
    if (sp)
        free(sp);
    if (abuf) {
        sg_audio_buffer_destroy(abuf);
        free(abuf);
    }
    return NULL;
}

static void
sg_mixer_sound_free(struct sg_mixer_sound *sound)
{
    free(sound->sample.data);
    free(sound);
}

void
sg_mixer_sound_incref(struct sg_mixer_sound *sound)
{
    if (sound)
        pce_atomic_inc(&sound->refcount);
}

void
sg_mixer_sound_decref(struct sg_mixer_sound *sound)
{
    if (sound && pce_atomic_fetch_add(&sound->refcount, -1) == 1)
        sg_mixer_sound_free(sound);
}
