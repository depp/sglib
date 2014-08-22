/* Copyright 2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "sound.h"
#include "sg/audio_file.h"
#include "sg/error.h"
#include "sg/file.h"
#include "sg/log.h"
#include "sg/mixer.h"
#include "sg/thread.h"
#include "sg/util.h"
#include <stdlib.h>
#include <string.h>

#define SG_MIXER_SOUND_MAXSZ (16 * 1024 * 1024)

struct sg_mixer_soundglobal {
    struct sg_logger *log;

    /* Lock for this structure.  */
    struct sg_lock lock;

    /* The sample rate for all sounds.  */
    int rate;

    /* Table of all sound objects.  */
    struct sg_mixer_sound **sound;
    unsigned soundcount;
    unsigned soundalloc;
};

static struct sg_mixer_soundglobal sg_mixer_soundglobal;

void
sg_mixer_sound_init(void)
{
    sg_mixer_soundglobal.log = sg_logger_get("resource");
    sg_lock_init(&sg_mixer_soundglobal.lock);
}

static void
sg_mixer_sound_unload(struct sg_mixer_sound *sound)
{
    sg_atomic_set(&sound->is_loaded, 0);
    free(sound->sample.data);
    sound->sample.data = NULL;
    sound->sample.stereo = 0;
    sound->sample.length = 0;
}

static void
sg_mixer_sound_load(struct sg_mixer_sound *sound, int rate)
{
    struct sg_audio_buffer *abuf = NULL;
    size_t abufcount = 0;
    struct sg_buffer *buf = NULL;
    struct sg_error *err = NULL;
    const char *why;
    int r;

    buf = sg_file_get(sound->path, sound->pathlen,
                      SG_RDONLY, SG_AUDIO_FILE_EXTENSIONS,
                      SG_MIXER_SOUND_MAXSZ, &err);
    if (!buf)
        goto err;

    r = sg_audio_file_load(&abuf, &abufcount, buf->data, buf->length, &err);
    if (r) {
        why = "failed to load file";
        goto err;
    }

    if (abufcount != 1) {
        why = "cannot load multiple ogg streams";
        goto err;
    }

    if (abuf->nchan != 1 && abuf->nchan != 2) {
        why = "too many channels";
        goto err;
    }

    r = sg_audio_buffer_convert(abuf, SG_AUDIO_S16NE, &err);
    if (r) {
        why = "could not convert sample format";
        goto err;
    }

    if (rate != abuf->rate) {
        r = sg_audio_buffer_resample(abuf, rate, &err);
        if (r) {
            why = "could not convert sample rate";
            goto err;
        }
    }

    sound->sample.data = sg_audio_buffer_detach(abuf, &err);
    if (!sound->sample.data) {
        why = NULL;
        goto err;
    }
    sound->sample.stereo = abuf->nchan == 2;
    sound->sample.length = abuf->nframe;
    sg_atomic_set_release(&sound->is_loaded, 1);

    sg_audio_buffer_destroy(abuf);
    sg_buffer_decref(buf);

    return;

err:
    if (why)
        sg_logerrf(sg_mixer_soundglobal.log, SG_LOG_ERROR, err,
                   "%s: %s", sound->path, why);
    else
        sg_logerrs(sg_mixer_soundglobal.log, SG_LOG_ERROR, err,
                   sound->path);
    sg_error_clear(&err);

    if (buf)
        sg_buffer_decref(buf);
    if (abuf) {
        sg_audio_buffer_destroy(abuf);
        free(abuf);
    }

    sg_atomic_set_release(&sound->is_loaded, 1);
}

void
sg_mixer_sound_setrate(int rate)
{
    struct sg_mixer_soundglobal *sg = &sg_mixer_soundglobal;
    struct sg_mixer_sound **tp, **te, *sp;

    sg_lock_acquire(&sg->lock);
    if (sg->rate != rate) {
        tp = sg->sound;
        te = tp + sg->soundcount;
        for (; tp != te; tp++) {
            sp = *tp;
            sg_mixer_sound_unload(sp);
            sg_mixer_sound_load(sp, rate);
        }
        sg->rate = rate;
    }
    sg_lock_release(&sg->lock);
}

struct sg_mixer_sound *
sg_mixer_sound_file(const char *path, size_t pathlen,
                    struct sg_error **err)
{
    struct sg_mixer_soundglobal *sg = &sg_mixer_soundglobal;
    struct sg_mixer_sound *sp, **tp, **te, **ntab;
    int npathlen, rate;
    unsigned nalloc;
    char npath[SG_MAX_PATH], *pp;

    npathlen = sg_path_norm(npath, path, pathlen, err);
    if (npathlen < 0)
        return NULL;

    sg_lock_acquire(&sg->lock);
    tp = sg->sound;
    te = tp + sg->soundcount;
    for (; tp != te; tp++) {
        sp = *tp;
        if (sp->pathlen == npathlen && !memcmp(npath, sp->path, npathlen)) {
            sg_atomic_inc(&sp->refcount);
            sg_lock_release(&sg->lock);
            return sp;
        }
    }

    sp = malloc(sizeof(*sp) + npathlen + 1);
    if (!sp)
        goto nomem;

    pp = (char *) (sp + 1);
    sg_atomic_set(&sp->refcount, 1);
    sg_atomic_set(&sp->is_loaded, 0);
    sp->path = pp;
    sp->pathlen = npathlen;
    sp->sample.data = NULL;
    sp->sample.stereo = 0;
    sp->sample.length = 0;
    memcpy(pp, npath, npathlen + 1);

    if (sg->soundcount >= sg->soundalloc) {
        nalloc = sg_round_up_pow2_32(sg->soundcount + 1);
        if (!nalloc)
            goto nomem;
        ntab = realloc(sg->sound, nalloc * sizeof(*ntab));
        if (!ntab)
            goto nomem;
        sg->sound = ntab;
        sg->soundalloc = nalloc;
    }

    sg->sound[sg->soundcount++] = sp;
    rate = sg->rate;
    /* FIXME: asynchronous loading: load the sound without the main
       lock held.  */
    sg_mixer_sound_load(sp, rate);
    sg_lock_release(&sg->lock);

    return sp;

nomem:
    sg_lock_release(&sg->lock);
    sg_error_nomem(err);
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
        sg_atomic_inc(&sound->refcount);
}

void
sg_mixer_sound_decref(struct sg_mixer_sound *sound)
{
    if (sound && sg_atomic_fetch_add(&sound->refcount, -1) == 1)
        sg_mixer_sound_free(sound);
}
