/* Copyright 2012 Dietrich Epp <depp@zdome.net> */
#include "libpce/hashtable.h"
#include "libpce/thread.h"
#include "sg/aio.h"
#include "sg/audio_pcm.h"
#include "sg/audio_sample.h"
#include "sg/dispatch.h"
#include "sg/error.h"
#include "sg/file.h"
#include "sg/log.h"
#include "sysprivate.h"
#include <stdlib.h>
#include <string.h>

struct sg_audio_sample_global {
    /* Lock for all the fields in this structure, as well as the
       `load` field in audio samples.  */
    struct pce_lock lock;

    /* Sample rate for all audio samples.  If this is not a positive
       number, then no samples will be loaded into memory.  */
    int rate;

    /* Table of all audio samples indexed by path.  This allows two
       requests for the same audio file to return the same object.  */
    struct pce_hashtable table;
};

static struct sg_audio_sample_global sg_audio_sample_global;

struct sg_audio_sample_load {
    const char *path;
    int pathlen;
    struct sg_audio_sample *sample;
    struct sg_aio_request *ioreq;
    struct sg_buffer *buf;
};

/* Finish an audio sample loading operation.

   This frees the audio sample load structure.  This should be called
   with the lock held.

   If err is not NULL and does not have the domain SG_ERROR_CANCEL,
   then an error is logged indicating that audio sample loading
   failed.  Otherwise, the loading operation was either completed
   successfully or cancelled successfully.  This function does not
   distinguish between a canceled operation and a successful
   operation, so err can be NULL when the operation is canceled.  */
static void
sg_audio_sample_finish(struct sg_audio_sample_load *lp, struct sg_error *err)
{
    if (err && err->domain != &SG_ERROR_CANCEL) {
        sg_logerrf(sg_logger_get("rsrc"), LOG_ERROR,
                   err, "%s: audio file failed to load", lp->path);
    }

    if (lp->sample)
        lp->sample->load = NULL;
    if (lp->buf)
        sg_buffer_decref(lp->buf);
    free(lp);
}

/* Audio sample loading callback when IO operation completes */
static void
sg_audio_sample_aiocb(void *cxt, struct sg_buffer *buf, struct sg_error *err);

/* Audio sample loading callback to decode file data */
static void
sg_audio_sample_decodecb(void *cxt);

static void
sg_audio_sample_aiocb(void *cxt, struct sg_buffer *buf, struct sg_error *err)
{
    struct sg_audio_sample_load *lp = cxt;
    struct sg_audio_sample_global *gp = &sg_audio_sample_global;
    pce_lock_acquire(&gp->lock);
    lp->ioreq = NULL;

    if (!lp->sample || !buf) {
        sg_audio_sample_finish(lp, err);
    } else {
        sg_buffer_incref(buf);
        lp->buf = buf;
        sg_dispatch_queue(0, lp, sg_audio_sample_decodecb);
    }

    pce_lock_release(&gp->lock);
}

static void
sg_audio_sample_decodecb(void *cxt)
{
    struct sg_audio_sample *sp;
    struct sg_audio_sample_load *lp = cxt;
    struct sg_audio_sample_global *gp = &sg_audio_sample_global;
    struct sg_buffer *buf = NULL;
    struct sg_audio_pcm *pcm = NULL;
    struct sg_error *err = NULL;
    short *data = NULL;
    unsigned flags;
    int r, rate, done;
    size_t pcmcount = 0, pcmi;

    pce_lock_acquire(&gp->lock);
    rate = gp->rate;
    if (!lp->sample || rate <= 0) {
        sg_audio_sample_finish(lp, NULL);
        pce_lock_release(&gp->lock);
        return;
    }
    buf = lp->buf;
    lp->buf = NULL;
    done = 0;
    while (!done) {
        pce_lock_release(&gp->lock);

        if (pcm) {
            for (pcmi = 0; pcmi < pcmcount; pcmi++)
                sg_audio_pcm_destroy(&pcm[pcmi]);
            free(pcm);
            pcm = NULL;
        }

        r = sg_audio_pcm_load(&pcm, &pcmcount, buf->data, buf->length, &err);
        if (r)
            goto finish;

        if (pcmcount != 1) {
            sg_logf(sg_logger_get("rsrc"), LOG_ERROR,
                    "%s: concatenated streams not supported");
            goto finish;
        }

        switch (pcm->nchan) {
        case 1:
            flags = 0;
            break;

        case 2:
            flags = SG_AUDIO_SAMPLE_STEREO;
            break;

        default:
            sg_logf(sg_logger_get("rsrc"), LOG_ERROR,
                    "%s: audio file has too many channels",
                    lp->path);
            goto finish;
        }

        r = sg_audio_pcm_convert(pcm, SG_AUDIO_S16NE, &err);
        if (r)
            goto finish;

        if (pcm->rate != rate) {
            r = sg_audio_pcm_resample(pcm, rate, &err);
            if (r)
                goto finish;
            flags |= SG_AUDIO_SAMPLE_RESAMPLED;
        }

        data = sg_audio_pcm_detach(pcm, &err);
        if (pcm->nframe && !data)
            goto finish;

        pce_lock_acquire(&gp->lock);
        sp = lp->sample;
        if (sp) {
            if (rate == gp->rate) {
                /* Load was successful.  */
                sp->flags = flags;
                sp->nframe = pcm->nframe;
                sp->playtime = (int)
                    (((long long) pcm->nframe * 1000 + pcm->rate - 1) /
                     pcm->rate);
                sp->rate = pcm->rate;
                sp->data = data;
                data = NULL;
                pce_atomic_set_release(&sp->loaded, 1);
                done = 1;
            } else {
                if (gp->rate > 0) {
                    /* The sample rate was changed during the loading
                       operation.  Try again.  */
                    rate = gp->rate;
                } else {
                    /* Audio system was shut down.  */
                    done = 1;
                }
            }
        } else {
            /* Loading operation was canceled.  */
            done = 1;
        }
    }
    sg_audio_sample_finish(lp, NULL);
    pce_lock_release(&gp->lock);

finish:
    if (!done) {
        pce_lock_acquire(&gp->lock);
        sg_audio_sample_finish(lp, NULL);
        pce_lock_release(&gp->lock);
    }

    free(data);
    sg_buffer_decref(buf);
    buf = NULL;
    if (pcm) {
        for (pcmi = 0; pcmi < pcmcount; pcmi++)
            sg_audio_pcm_destroy(&pcm[pcmi]);
        free(pcm);
    }
}

/* Load or reload the given audio sample.  The lock must be held, and
   a loading operation must not be in progress for the sample.  The
   audio sample will be loaded in the background.  If this function
   fails, it will simply log an error and return.  */
static void
sg_audio_sample_load(struct sg_audio_sample *sp)
{
    struct sg_audio_sample_load *lp;
    struct sg_aio_request *ioreq;
    struct sg_error *err = NULL;
    char *pp;

    lp = malloc(sizeof(*lp) + sp->namelen + 1);
    if (!lp) {
        sg_error_nomem(&err);
        goto error;
    }

    ioreq = sg_aio_request(
        sp->name, sp->namelen, 0,
        SG_AUDIO_PCM_EXTENSIONS,
        SG_AUDIO_PCM_MAXFILESZ,
        lp, sg_audio_sample_aiocb,
        &err);
    if (!ioreq)
        goto error;

    pp = (char *) (lp + 1);
    memcpy(pp, sp->name, sp->namelen + 1);
    lp->path = pp;
    lp->pathlen = sp->namelen;
    lp->sample = sp;
    lp->ioreq = ioreq;
    lp->buf = NULL;

    sp->load = lp;
    return;

error:
    sg_logerrf(sg_logger_get("rsrc"), LOG_ERROR,
               err, "%s: audio file failed to load", sp->name);
    free(lp);
    sg_error_clear(&err);
}

void
sg_audio_sample_init(void)
{
    pce_lock_init(&sg_audio_sample_global.lock);
}

void
sg_audio_sample_setrate(int rate)
{
    struct sg_audio_sample_global *gp = &sg_audio_sample_global;
    struct pce_hashtable_entry *ep, *ee;
    struct sg_audio_sample *sp;
    pce_lock_acquire(&gp->lock);
    gp->rate = rate;

    ep = gp->table.contents;
    ee = ep + gp->table.capacity;
    for (; ep != ee; ep++) {
        sp = ep->value;
        if (!sp || sp->load)
            continue;

        if (rate > 0) {
            if (!pce_atomic_get(&sp->loaded)) {
                sg_audio_sample_load(sp);
            } else if (sp->rate != rate) {
                pce_atomic_set(&sp->loaded, 0);
                free(sp->data);
                sp->data = NULL;
                sg_audio_sample_load(sp);
            }
        } else {
            if (pce_atomic_get(&sp->loaded)) {
                pce_atomic_set(&sp->loaded, 0);
                free(sp->data);
                sp->data = NULL;
            }
        }
    }

    pce_lock_release(&gp->lock);
}

void
sg_audio_sample_free_(struct sg_audio_sample *sp)
{
    struct sg_audio_sample_global *gp = &sg_audio_sample_global;
    struct sg_audio_sample_load *lp;
    struct pce_hashtable_entry *ep;

    pce_lock_acquire(&gp->lock);

    /* Cancel any load in progress */
    lp = sp->load;
    if (lp) {
        if (lp->ioreq)
            sg_aio_cancel(lp->ioreq);
        lp->sample = NULL;
    }

    /* Remove audio sample from global table.  If the same audio file
       was loaded after the reference count hit zero but before we
       acquired the lock, a different sample will be in the global
       table.  */
    ep = pce_hashtable_get(&gp->table, sp->name);
    if (ep && ep->value == sp)
        pce_hashtable_erase(&gp->table, ep);

    pce_lock_release(&gp->lock);

    free(sp->data);
    free(sp);
}

struct sg_audio_sample *
sg_audio_sample_file(const char *path, size_t pathlen,
                     struct sg_error **err)
{
    struct sg_audio_sample_global *gp = &sg_audio_sample_global;
    struct pce_hashtable_entry *ep = NULL;
    struct sg_audio_sample *sp = NULL;
    char npath[SG_MAX_PATH], *pp;
    int npathlen, r;

    npathlen = sg_path_norm(npath, path, pathlen, err);
    if (npathlen < 0)
        return NULL;

    pce_lock_acquire(&gp->lock);
    ep = pce_hashtable_insert(&gp->table, npath);
    if (!ep)
        goto nomem;

    sp = ep->value;
    if (sp) {
        /* If the reference count was zero, then another thread is
           deleting the audio sample and waiting on the global
           lock.  */
        r = pce_atomic_fetch_add(&sp->refcount, 1);
        if (r > 0) {
            pce_lock_release(&gp->lock);
            return sp;
        }
    }

    sp = malloc(sizeof(*sp) + npathlen + 1);
    if (!sp)
        goto nomem;

    pp = (char *) (sp + 1);
    memcpy(pp, npath, npathlen + 1);
    pce_atomic_set(&sp->refcount, 1);
    sp->load = NULL;
    sp->name = pp;
    sp->namelen = npathlen;
    pce_atomic_set(&sp->loaded, 0);
    sp->flags = 0;
    sp->nframe = 0;
    sp->playtime = 0;
    sp->rate = 0;
    sp->data = NULL;

    ep->key = pp;
    ep->value = sp;

    if (gp->rate > 0)
        sg_audio_sample_load(sp);

    pce_lock_release(&gp->lock);
    return sp;

nomem:
    sg_error_nomem(err);
    goto error;

error:
    if (ep)
        pce_hashtable_erase(&gp->table, ep);
    free(sp);
    pce_lock_release(&gp->lock);
    return NULL;
}
