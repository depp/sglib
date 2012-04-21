#include "audio.h"
#include "error.h"
#include "thread.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SOUNDS 16

struct sg_audio_systemsrc {
    struct sg_audio_source *src;
    int is_active;
    unsigned activation_time;
};

struct sg_audio_system {
    struct sg_audio_system *next;
    int rate;

    /* Fields below here are accessed through the lock */
    struct sg_lock lock;
    unsigned nsource, asource;
    struct sg_audio_systemsrc *sources;
    float *mixbuf;
    int mixbufsz;
};

struct sg_audio_system *sg_audio_system_first;

int
sg_audio_play(unsigned time, void *obj, sg_audio_source_f func,
              struct sg_error **err)
{
    struct sg_audio_system *sp = sg_audio_system_first;
    struct sg_audio_source *src;
    struct sg_audio_systemsrc *ssp;
    unsigned nalloc;
    for (; sp; sp = sp->next) {
        src = func(obj, sp->rate, err);
        if (!src)
            return -1;
        sg_lock_acquire(&sp->lock);
        if (sp->nsource >= MAX_SOUNDS) {
            src->free(src);
            sg_lock_release(&sp->lock);
            continue;
        }
        if (sp->nsource >= sp->asource) {
            nalloc = sp->nsource ? sp->nsource * 2 : 2;
            ssp = realloc(sp->sources, sizeof(*ssp) * nalloc);
            if (!sp) {
                sg_lock_release(&sp->lock);
                src->free(src);
                sg_error_nomem(err);
                return -1;
            }
            sp->sources = ssp;
        }
        ssp = &sp->sources[sp->nsource++];
        ssp->src = src;
        ssp->is_active = 0;
        ssp->activation_time = time;
        sg_lock_release(&sp->lock);
    }
    return 0;
}

struct sg_audio_system *
sg_audio_system_new(int rate, struct sg_error **err)
{
    struct sg_audio_system *sys;

    sys = malloc(sizeof(*sys));
    if (!sys) {
        sg_error_nomem(err);
        return NULL;
    }
    sys->next = sg_audio_system_first;
    sys->rate = rate;
    sg_lock_init(&sys->lock);
    sys->nsource = 0;
    sys->asource = 0;
    sys->sources = NULL;
    sys->mixbuf = NULL;
    sys->mixbufsz = 0;
    sg_audio_system_first = sys;

    return sys;
}

void
sg_audio_system_free(struct sg_audio_system *sp)
{
    struct sg_audio_system *last, *sys;
    struct sg_audio_systemsrc *ssrcs, *ssrc, *ssrce;
    struct sg_audio_source *src;
    last = NULL;
    sys = sg_audio_system_first;
    while (sys != sp) {
        assert(sys);
        last = sys;
        sys = sys->next;
    }
    if (last)
        last->next = sp->next;
    else
        sg_audio_system_first = sp;
    ssrcs = sp->sources;
    ssrce = ssrcs + sp->nsource;
    for (ssrc = ssrcs; ssrc != ssrce; ++ssrc) {
        src = ssrc->src;
        src->free(src);
    }
    free(ssrcs);
    free(sp->mixbuf);
    sg_lock_destroy(&sp->lock);
    free(sp);
}

void
sg_audio_system_pull(struct sg_audio_system *sp,
                     unsigned time, float *buf, int nframes)
{
    float *mixbuf;
    struct sg_audio_systemsrc *ssrcs, *ssrc, *ssrce, *ssrco;
    struct sg_audio_source *src;
    int i, sframes, rate = sp->rate;
    unsigned dtime = (nframes * 1000) / rate, delay_ms, delay_frame;

    for (i = 0; i < nframes; ++i) {
        buf[i*2+0] = 0.0f;
        buf[i*2+1] = 0.0f;
    }
    sg_lock_acquire(&sp->lock);

    if (sp->mixbufsz < nframes) {
        free(sp->mixbuf);
        sp->mixbuf = NULL;
        mixbuf = malloc(sizeof(*mixbuf) * nframes * 2);
        if (!mixbuf) {
            sp->mixbufsz = 0;
            goto unlock;
        }
        sp->mixbuf = mixbuf;
        sp->mixbufsz = nframes;
    } else {
        mixbuf = sp->mixbuf;
    }
    ssrcs = sp->sources;
    ssrce = ssrcs + sp->nsource;

    for (ssrc = ssrcs; ssrc != ssrce; ++ssrc) {
        if (0 && !ssrc->is_active) {
            delay_ms = ssrc->activation_time - time;
            if (delay_ms + dtime >= dtime + dtime)
                continue;
            if (delay_ms > dtime)
                delay_ms = 0;
            delay_frame = (rate * delay_ms) / 1000;
            ssrc->is_active = 1;
            if (delay_frame >= (unsigned) nframes)
                continue;
        } else {
            delay_frame = 0;
        }
        src = ssrc->src;
        sframes = src->fill(src, mixbuf + delay_frame,
                            nframes - delay_frame);
        if (sframes > 0) {
            for (i = 0; i < sframes; ++i) {
                buf[delay_frame*2 + i*2 + 0] += mixbuf[i*2+0];
                buf[delay_frame*2 + i*2 + 1] += mixbuf[i*2+1];
            }
        }
    }

    for (ssrc = ssrcs; ssrc != ssrce; ++ssrc) {
        src = ssrc->src;
        if (src->flags & SG_AUDIO_TERMINATE) {
            src->free(src);
            ssrco = ssrc;
            for (++ssrc; ssrc != ssrce; ++ssrc) {
                src = ssrc->src;
                if (src->flags & SG_AUDIO_TERMINATE) {
                    src->free(src);
                } else {
                    memcpy(ssrco, ssrc, sizeof(*ssrc));
                    ssrco++;
                }
            }
            sp->nsource = ssrco - ssrcs;
            break;
        }
    }

unlock:
    sg_lock_release(&sp->lock);
}
