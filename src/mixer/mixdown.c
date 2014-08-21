/* Copyright 2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "mixer.h"
#include "sound.h"
#include "sg/error.h"
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

static struct sg_mixer_mixdowniface *
sg_mixer_mixdown_new(sg_mixer_which_t which, int bufsz)
{
    struct sg_mixer_mixdowniface *mi;
    struct sg_mixer_mixdown *mp;
    unsigned channelcount = sg_mixer.channelcount;

    mi = malloc(sizeof(*mi));
    if (!mi)
        return NULL;

    sg_mixer_queue_init(&mi->inqueue);
    mp = &mi->mixdown;

    sg_mixer_queue_init(&mp->procqueue);
    mp->which = which;
    mp->channel = malloc(sizeof(*mp->channel) * channelcount);
    if (!mp->channel)
        goto nomem1;
    mp->channelcount = channelcount;
    mp->bufsz = bufsz;
    mp->audio_buf = calloc(sizeof(float), bufsz * 4);
    if (!mp->audio_buf)
        goto nomem2;
    mp->param_buf = malloc(
        sizeof(float) * (bufsz >> SG_MIXER_PARAMRATE) * 2);
    if (!mp->param_buf)
        goto nomem3;

    return mi;

nomem3: free(mp->audio_buf);
nomem2: free(mp->channel);
nomem1: free(mp);
    return NULL;
}

struct sg_mixer_mixdowniface *
sg_mixer_mixdown_create_live(int samplerate, int bufsz,
                             struct sg_error **err)
{
    struct sg_mixer_mixdowniface *mi;

    mi = sg_mixer_mixdown_new(SG_MIXER_LIVE, bufsz);
    if (!mi) {
        sg_error_nomem(err);
        return NULL;
    }
    sg_mixer_time_init(&mi->mixdown.time.delayed, samplerate, bufsz);

    pce_lock_acquire(&sg_mixer.lock);
    /* This function is only called by the audio system, so this
       should not happen.  */
    if (sg_mixer.mix_live != NULL)
        abort();
    sg_mixer.mix_live = mi;
    pce_lock_release(&sg_mixer.lock);

    return mi;
}

void
sg_mixer_mixdown_destroy(struct sg_mixer_mixdowniface *mp)
{
    pce_lock_acquire(&sg_mixer.lock);
    if (mp->mixdown.which == SG_MIXER_LIVE) {
        assert(sg_mixer.mix_live == mp);
        sg_mixer.mix_live = NULL;
    } else {
        assert(sg_mixer.mix_record == mp);
        sg_mixer.mix_record = NULL;
    }
    pce_lock_release(&sg_mixer.lock);

    free(mp->mixdown.param_buf);
    free(mp->mixdown.audio_buf);
    free(mp->mixdown.channel);
    free(mp);
}

/* Synchronize channel flags.  Requires global mixer lock.  */
static void
sg_mixer_mixdown_syncflags(struct sg_mixer_mixdowniface *mp)
{
    struct sg_mixer_channel *gchan = sg_mixer.channel;
    struct sg_mixer_mixchan *mchan = mp->mixdown.channel;
    unsigned i, n = sg_mixer.channelcount, done, mask;
    done = mp->mixdown.which == SG_MIXER_LIVE ?
        SG_MIXER_GFLAG_DONE1 : SG_MIXER_GFLAG_DONE2;
    mask = done | SG_MIXER_GFLAG_START;
    for (i = 0; i < n; i++) {
        if ((gchan[i].gflags & mask) != SG_MIXER_GFLAG_START)
            continue;
        if (mchan[i].flags & SG_MIXER_LFLAG_DONE) {
            gchan[i].gflags |= done;
            mchan[i].flags = 0;
            continue;
        }
        if (mchan[i].flags & SG_MIXER_LFLAG_START)
            mchan[i].flags &= ~SG_MIXER_LFLAG_INIT;
        else
            mchan[i].flags |= SG_MIXER_LFLAG_INIT;
        mchan[i].flags |= SG_MIXER_LFLAG_START;
        if (gchan[i].gflags & SG_MIXER_GFLAG_STOP)
            mchan[i].flags |= SG_MIXER_LFLAG_STOP;
    }
}

/* Collect incoming messages.  Requires global mixer lock.  */
static void
sg_mixer_mixdown_collect(struct sg_mixer_mixdowniface *mp)
{
    unsigned nmsg;
    struct sg_mixer_msg *msg;
    nmsg = mp->inqueue.msgcount;
    msg = sg_mixer_queue_append(&mp->mixdown.procqueue, nmsg);
    if (msg)
        memcpy(msg, mp->inqueue.msg, sizeof(*msg) * nmsg);
    mp->inqueue.msgcount += nmsg;
}

/* Sort messages by destination.  */
static void
sg_mixer_mixdown_sortmsg(struct sg_mixer_msg *SG_RESTRICT msg,
                         unsigned nmsg)
{
    struct sg_mixer_msg tmp;
    unsigned i, j;

    /* Dumb insertion sort.  Queue is probably small.  Optimized for
       case where the beginning of the queue is already sorted.  */
    for (i = 1; i < nmsg && msg[i - 1].addr <= msg[i].addr; i++) { }
    for (; i < nmsg; i++) {
        tmp = msg[i];
        for (j = i; j > 0 && msg[j - 1].addr > tmp.addr; j--)
            msg[j] = msg[j - 1];
        msg[j] = tmp;
    }
}

/* Zero the accumulation buffer for all busses.  */
static void
sg_mixer_mixdown_zero(struct sg_mixer_mixdown *SG_RESTRICT mp)
{
    int bus, apos, asz = mp->bufsz;
    float *abuf = mp->audio_buf;
    for (bus = 0; bus < 2; bus++) {
        for (apos = 0; apos < asz; apos++)
            abuf[apos + (bus + 2) * asz] = 0.0f;
    }
}

/* Render channel parameters.  The input parameters are dB gain and
   volume, and they need to be converted to linear gain.  */
static void
sg_mixer_mixdown_renderparam(struct sg_mixer_mixdown *SG_RESTRICT mp)
{
    int ppos, psz = mp->bufsz >> SG_MIXER_PARAMRATE;
    float *SG_RESTRICT pbuf = mp->param_buf;
    float vol, pan, volscale, panscale, gain;

    panscale = atanf(1.0f);
    volscale = logf(10.0f) / 20.0f;

    for (ppos = 0; ppos < psz; ppos++) {
        vol = pbuf[ppos + psz * 0];
        pan = pbuf[ppos + psz * 1];
        gain = exp(vol * volscale);
        if (vol < -60.0f)
            gain *= (vol + 80.0f) * (1.0f / 20.0f);
        pbuf[ppos + psz * 0] = gain * sinf((1.0f - pan) * panscale);
        pbuf[ppos + psz * 1] = gain * sinf((1.0f + pan) * panscale);
    }
}

/* Render a channel's audio to the channel input buffer.  */
static void
sg_mixer_mixdown_renderinput(struct sg_mixer_mixdown *SG_RESTRICT mp,
                             int ch, int start, int end)
{
    struct sg_mixer_sample *sample =
        &sg_mixer.channel[ch].sound->sample;
    const short *adata = sample->data;
    int apos, asz = mp->bufsz;
    int rem = (int) (sample->length - mp->channel[ch].samplepos);
    unsigned spos = mp->channel[ch].samplepos;
    float v, *abuf = mp->audio_buf;

    if (rem <= end - start) {
        end = start + rem;
        mp->channel[ch].flags |= SG_MIXER_LFLAG_DONE;
    }

    apos = 0;
    for (; apos < start; apos++) {
        abuf[apos + asz * 0] = 0.0f;
        abuf[apos + asz * 1] = 0.0f;
    }
    /* FIXME: Performance improvement: use SSE.  */
    if (sample->stereo) {
        adata += spos * 2;
        for (; apos < end; apos++, adata += 2) {
            abuf[apos + asz * 0] = (1.0f / 32768.0f) * (float) adata[0];
            abuf[apos + asz * 1] = (1.0f / 32768.0f) * (float) adata[1];
        }
    } else {
        /* FIXME: Performance improvement: we don't need to render to
           two scratch spaces if the sample is mono.  */
        adata += spos;
        for (; apos < end; apos++, adata++) {
            v = (1.0f / 32768.0f) * (float) adata[0];
            abuf[apos + asz * 0] = v;
            abuf[apos + asz * 1] = v;
        }
    }
    for (; apos < asz; apos++) {
        abuf[apos + asz * 0] = 0.0f;
        abuf[apos + asz * 1] = 0.0f;
    }

    mp->channel[ch].samplepos = spos + (end - start);
}

/* Render the current channel input buffer and parameter buffer to the
   bus outputs.  */
static void
sg_mixer_mixdown_renderoutput(struct sg_mixer_mixdown *SG_RESTRICT mp)
{
    float *SG_RESTRICT abuf = mp->audio_buf;
    float *SG_RESTRICT pbuf = mp->param_buf;
    float gain;
    int asz = mp->bufsz, psz = asz >> SG_MIXER_PARAMRATE;
    int ppos, apos, bus;

    for (bus = 0; bus < 2; bus++) {
        for (apos = 0; apos < asz; apos++) {
            /* FIXME: Quality improvement: use B-splines for
               interpolating bus gain.  */
            /* FIXME: Performance improvement: use SSE.  */
            ppos = apos >> SG_MIXER_PARAMRATE;
            gain = pbuf[ppos + psz * bus];
            abuf[apos + asz * (bus + 2)] += gain * abuf[apos + asz * bus];
        }
    }
}

/* Render mixdown audio.  */
static void
sg_mixer_mixdown_render(struct sg_mixer_mixdown *SG_RESTRICT mp)
{
    struct sg_mixer_msg *SG_RESTRICT msg = mp->procqueue.msg;
    unsigned nmsg = mp->procqueue.msgcount, nch = sg_mixer.channelcount;
    unsigned i, j, ch, param, addr, flags;
    sg_mixer_which_t which = mp->which;
    int asz = mp->bufsz, psz = asz >> SG_MIXER_PARAMRATE;
    int ppos, msgtime, starttime, stoptime;
    float *pbuf = mp->param_buf;
    float paramval;

    sg_mixer_mixdown_zero(mp);

    /* Render each channel's audio.  */
    sg_mixer_mixdown_sortmsg(msg, nmsg);
    i = 0;
    j = 0;
    for (ch = 0; ch < nch; ch++) {
        /* Note: DONE is always clear here, since it is cleared by
           syncflags(), which is called before this function.  */
        flags = mp->channel[ch].flags;
        if (!(flags & SG_MIXER_LFLAG_START)) {
            /* Discard messages.  */
            for (; i < nmsg && (msg[i].addr >> 16) == ch; i++) { }
            continue;
        }

        /* Start channel playback.  */
        if (!(flags & SG_MIXER_LFLAG_STARTED)) {
            if (which == SG_MIXER_LIVE)
                starttime = sg_mixer_time_get(
                    &mp->time.delayed, sg_mixer.channel[ch].starttime);
            else
                starttime = sg_mixer_timeexact_get(
                    &mp->time.exact, sg_mixer.channel[ch].starttime);
            if (starttime < asz) {
                if (starttime < 0)
                    starttime = 0;
                mp->channel[ch].samplepos = 0;
                for (param = 0; param < SG_MIXER_PARAM_COUNT; param++)
                    mp->channel[ch].param[param] =
                        sg_mixer.channel[ch].param_init[param];
            } else {
                /* Keep messages for later.  */
                for (; i < nmsg && (msg[i].addr >> 16) == ch; i++)
                    msg[j] = msg[i];
                continue;
            }
        } else {
            starttime = 0;
        }

        /* Stop channel playback.  */
        if (flags & SG_MIXER_LFLAG_STOP) {
            /* FIXME: Quality improvement: sounds which stop should
               fade out quickly instead of stopping abruptly.  */
            if (which == SG_MIXER_LIVE)
                stoptime = sg_mixer_time_get(
                    &mp->time.delayed, sg_mixer.channel[ch].stoptime);
            else
                stoptime = sg_mixer_timeexact_get(
                    &mp->time.exact, sg_mixer.channel[ch].stoptime);
            if (stoptime < asz) {
                if (stoptime < 0)
                    stoptime = 0;
                mp->channel[ch].flags |= SG_MIXER_LFLAG_DONE;
            } else {
                stoptime = asz;
            }
        } else {
            starttime = asz;
        }

        /* Render parameter changes into parameter sample buffer.  */
        for (param = 0; param < SG_MIXER_PARAM_COUNT; param++) {
            addr = (ch << 16) | param;
            paramval = mp->channel[ch].param[param];
            for (; i < nmsg && msg[i].addr == addr; i++) {
                if (which == SG_MIXER_LIVE)
                    msgtime = sg_mixer_time_get(
                        &mp->time.delayed, msg[i].timestamp);
                else
                    msgtime = sg_mixer_timeexact_get(
                        &mp->time.exact, msg[i].timestamp);
                if (msgtime >= asz) {
                    do msg[j++] = msg[i++];
                    while (i < nmsg && msg[i].addr == addr);
                    break;
                }
                msgtime = msgtime >> SG_MIXER_PARAMRATE;
                for (; ppos < msgtime; ppos++)
                    pbuf[ppos + psz * param] = paramval;
                paramval = msg[i].value;
            }
            for (; ppos < psz; ppos++)
                pbuf[ppos + psz * param] = paramval;
            mp->channel[ch].param[param] = paramval;
        }

        /* FIXME: Quality improvement: filter parameter changes.  */
        sg_mixer_mixdown_renderparam(mp);
        sg_mixer_mixdown_renderinput(mp, ch, starttime, stoptime);
        sg_mixer_mixdown_renderoutput(mp);
    }
    mp->procqueue.msgcount = j;
}

int
sg_mixer_mixdown_process(struct sg_mixer_mixdowniface *mp,
                         unsigned buffertime)
{
    pce_lock_acquire(&sg_mixer.lock);
    if (!sg_mixer.is_ready) {
        pce_lock_release(&sg_mixer.lock);
        return mp->mixdown.bufsz;
    }
    sg_mixer_mixdown_syncflags(mp);
    sg_mixer_mixdown_collect(mp);
    if (mp->mixdown.which == SG_MIXER_LIVE)
        sg_mixer_time_update(&mp->mixdown.time.delayed,
                             sg_mixer.committime, buffertime);
    else
        sg_mixer_timeexact_update(&mp->mixdown.time.exact);
    pce_lock_release(&sg_mixer.lock);

    sg_mixer_mixdown_render(&mp->mixdown);

    return mp->mixdown.bufsz;
}

void
sg_mixer_mixdown_get_f32(struct sg_mixer_mixdowniface *mp,
                         float *buffer)
{
    int apos, asz = mp->mixdown.bufsz;
    const float *SG_RESTRICT input = mp->mixdown.audio_buf;
    float *SG_RESTRICT output = buffer;
    /* FIXME: Performance improvement: use SSE */
    for (apos = 0; apos < asz; apos++) {
        output[apos * 2 + 0] = input[apos + asz * 2];
        output[apos * 2 + 1] = input[apos + asz * 3];
    }
}
