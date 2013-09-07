/* Copyright 2012-2013 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "libpce/util.h"
#include "sg/audio_mixdown.h"
#include "sg/audio_pcm.h"
#include "sg/error.h"
#include "sg/log.h"
#include "sysprivate.h"
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <stdio.h>

/* Maximum size of message queue */
#define SG_AUDIO_MIXMAXBUF 4096

/* Ratio of parameter sample buffers to audio sample buffers */
#define SG_AUDIO_PARAMBITS 5
#define SG_AUDIO_PARAMRATE (1 << (SG_AUDIO_PARAMBITS))

/* Time, in seconds, which it takes a stopped sound to fade out */
#define SG_AUDIO_FADETIME 0.1f

/* Rate at which audio sample fade out, in dB/sec */
#define SG_AUDIO_FADERATE (-SG_AUDIO_SILENT / SG_AUDIO_FADETIME)

struct sg_audio_mixsrc {
    int chan;
    float params[SG_AUDIO_PARAMCOUNT];
};

/* A mix parameter records the most recent segment of parameter
   automation.  It records a value going from val[0] at pos[0], to
   val[1] at pos[1].  The positions are measured in samples.

   In the sample buffer for parameters, positions prior to pos[0] are
   already filled with samples.  */
struct sg_audio_mixparam {
    int pos[2];
    float val[2];
};

struct sg_audio_mixchan {
    unsigned flags;
    int src;
    struct sg_audio_pcm_obj *sample;

    /* The sample position in the sample.  This is measured using the
       mixdown's sample rate, and is relative to the start of the
       current buffer.  */
    int pos;

    struct sg_audio_mixparam params[SG_AUDIO_PARAMCOUNT];
};

#define SG_AUDIO_MIXDTBITS 9
#define SG_AUDIO_MIXDT (1 << (SG_AUDIO_MIXDTBITS))
#define NTIMESAMP 3

/* Mixdown timing algorithm */
typedef enum {
    /* Automatially minimize latency */
    SG_AUDIO_LIVE,
    /* Block until sounds are ready */
    SG_AUDIO_OFFLINE
} sg_audio_mixdowntype_t;

struct sg_audio_mixdown {
    /* Pointer to free in order to free the mixdown */
    void *root;

    /* ========== General info ========== */

    /* Audio mixdown type: LIVE or OFFLINE */
    sg_audio_mixdowntype_t type;

    /* Nominal audio sample rate, in Hz */
    int samplerate;

    /* Audio buffer size */
    int abufsize;

    /* Audio buffer margin / mixahead, in samples.  This is the safety
       margin to apply on top of the existing latency.  The system
       will try to line up the commit time with the end of the buffer,
       plus this many samples.  */
    int mixahead;

    /* Index of the mixdown.  Each mixdown has a unique index, and the
       index corresponds to a position in the 'mixpos' array of the
       audio system structure.  */
    int index;

    /* ========== Message buffers ========== */

    /* Buffer of most recently received messages from the global
       queue */
    void *mbuf;
    unsigned mbufsize;
    unsigned mbufalloc;

    /* Amount to advance the read pointer: set by getmsg, read by
       updatepos */
    unsigned advance;

    /* ========== Timing info ========== */

    /* Wall time and commit time of most recently received messages
       from global queue */
    unsigned wtime;
    unsigned ctime;

    /* Reference time: corresponds to the sample position in
       timestamp[0] */
    unsigned timeref;

    /*
      For LIVE audio:

      Map from timestamps to sample positions: the sample position,
      relative to the beginning of the current buffer, of the given
      times.  The sample in 'timestamp[0]' corresponds to the
      timestamp in 'timeref', and each next entry in the array
      corresponds to SG_AUDIO_MIXDT ticks earlier.

      For OFFLINE audio:

      The sample position in timestamp[0] corresponds to the timestamp
      in timeref, but the remaining entries are not used.
    */
    int timesamp[NTIMESAMP];

    /* For LIVE audio: Accumulators for solving for timestamps */
    double tm_x, tm_y, tm_xx, tm_xy;
    int tm_n;

    /* For LIVE audio: Filtered (average) dt per sample */
    double ta_avgdt, ta_avgdt0;
    unsigned ta_tprev, ta_tc, ta_tn;

    /* ========== Audio data ========== */

    /* Array mapping source ids (from the global queue) to mixer
       channels */
    struct sg_audio_mixsrc *srcs;
    unsigned srccount;

    /* Array of mixer channels */
    struct sg_audio_mixchan *chans;
    unsigned chancount;
    /* Index of first channel in freelist */
    int chanfree;

    /* ========== Sample buffers ========== */
    float *buf_mix;
    float *buf_samp;
    float *buf_param;
};

/* ========================================
   Timing
   ======================================== */

/*
  Convert a timestamp to a sample position.
*/
static int
sg_audio_mixdown_t2s(struct sg_audio_mixdown *SG_RESTRICT mp,
                     unsigned time)
{
    int dt, s0, s1, frac, sec;

    if (mp->type == SG_AUDIO_OFFLINE) {
        dt = time - mp->timeref;
        sec = dt / 1000;
        frac = dt % 1000;
        return mp->timesamp[0] + sec * mp->samplerate +
            (frac * mp->samplerate) / 1000;
    }

    dt = time - mp->timeref + SG_AUDIO_MIXDT * 2;
    if (dt < SG_AUDIO_MIXDT) {
        if (dt <= 0)
            return mp->timesamp[2];
        s0 = mp->timesamp[1];
        s1 = mp->timesamp[2];
    } else {
        dt -= SG_AUDIO_MIXDT;
        if (dt >= SG_AUDIO_MIXDT)
            return mp->timesamp[0];
        s0 = mp->timesamp[0];
        s1 = mp->timesamp[1];
    }
    return (s0 * dt + s1 * (SG_AUDIO_MIXDT - dt)) >> SG_AUDIO_MIXDTBITS;
}

/*
  Update the mixdown time system, which maps timestamps (measured in
  milliseconds) to buffer positions.
*/
static void
sg_audio_mixdown_updatetime(struct sg_audio_mixdown *SG_RESTRICT mp,
                            unsigned time)
{
    unsigned curtime;
    int dti, dsi, i, ns, ni;
    double ds, dt, m, b, n, s, a;

    curtime = mp->ctime;
    (void) time;

    if (mp->type == SG_AUDIO_OFFLINE) {
        mp->timesamp[0] -= mp->abufsize;
        if (mp->timesamp[0] < -mp->samplerate / 2) {
            mp->timesamp[0] += mp->samplerate;
            mp->timeref += 1000;
        }
        return;
    }

    if (mp->tm_n <= 0) {
        if (mp->tm_n < 0) {
            mp->timeref = curtime + SG_AUDIO_MIXDT;
            dsi = (int) ((SG_AUDIO_MIXDT * 0.001) * mp->samplerate);
            for (i = 0; i < NTIMESAMP; ++i)
                mp->timesamp[i] = dsi * (1 - i) +
                    mp->abufsize * 2 + mp->mixahead;

            dt = 1000.0 / mp->samplerate;
            mp->ta_avgdt = dt;
            mp->ta_avgdt0 = dt;
            mp->ta_tprev = curtime - (int) (mp->abufsize * dt);
            mp->ta_tc = (mp->samplerate + mp->abufsize/2) / mp->abufsize;
            if (mp->ta_tc < 1)
                mp->ta_tc = 1;
            mp->ta_tn = mp->ta_tc;
        }

        mp->tm_x = 0.0;
        mp->tm_y = 0.0;
        mp->tm_xx = 0.0;
        mp->tm_xy = 0.0;
        mp->tm_n = 0;
    }

    mp->ta_tn -= 1;
    if (!mp->ta_tn) {
        mp->ta_tn = mp->ta_tc;
        dt = (double) (mp->wtime - mp->ta_tprev) /
            (mp->ta_tc * mp->abufsize);
        mp->ta_tprev = mp->wtime;
        a = 0.125;
        mp->ta_avgdt0 = dt = a * dt + (1.0 - a) * mp->ta_avgdt0;
        mp->ta_avgdt = dt = a * dt + (1.0 - a) * mp->ta_avgdt;
        // printf("rate: %.2f\n", 1000.0 / dt);
    }

    ni = ++mp->tm_n;
    dti = curtime - mp->timeref;
    dsi = ni * mp->abufsize; // + mp->abufsize / 2;
    mp->tm_n = ni;

    dt = (double) dti;
    ds = (double) dsi;
    mp->tm_x += ds;
    mp->tm_y += dt;
    mp->tm_xx += ds * ds;
    mp->tm_xy += ds * dt;

    for (i = 0; i < NTIMESAMP; ++i)
        mp->timesamp[i] -= mp->abufsize;

    if (dti < 0)
        return;

    /* dsi = average # samples per 2*MIXDT */
    dsi = (int) (2 * SG_AUDIO_MIXDT / mp->ta_avgdt);
    if (ni > 2) {
        /* If we have enough data, do a linear regression to find the
           coefficients for the equation (t = m * s + b) which
           converts ticks to samples.  Solve for s at t=dt using these
           coefficients, then use the average slope to guess s at
           t=2dt.  */
        n = ni;
        m = (n * mp->tm_xy - mp->tm_x * mp->tm_y) /
            (n * mp->tm_xx - mp->tm_x * mp->tm_x);
        b = (mp->tm_y - m * mp->tm_x) / n;
        if (m * mp->samplerate < 500.0) {
            ns = ni * mp->abufsize;
        } else {
            s = -b / m;
            // printf("XX %d\n", (int) s - (int) ni * mp->abufsize);
            ns = (s > 0) ? (int) s : 0;
        }

        ns += dsi / 2;
    } else {
        /* Otherwise, use the average slope to make a guess for s at
           t=2dt.  */
        ns = dsi;
    }

    for (i = NTIMESAMP; i > 1; --i)
        mp->timesamp[i - 1] = mp->timesamp[i - 2];
    mp->timesamp[0] = ns - ((ni - 1) * mp->abufsize) + mp->mixahead;

    if (0) {
        fputs("timesamp:", stderr);
        for (i = 0; i < NTIMESAMP; ++i)
            printf(" %6d", mp->timesamp[i]);
        putchar('\n');
    }

    mp->tm_n = 0;
    mp->timeref += SG_AUDIO_MIXDT;
}

/* ========================================
   Communication with global queue
   ======================================== */

/*
  Get the latest messages from the global queue, and append them to
  the local queue.  This will NOT update the read position.
*/
static void
sg_audio_mixdown_getmsg(struct sg_audio_mixdown *SG_RESTRICT mp)
{
    struct sg_audio_system *SG_RESTRICT sp = &sg_audio_system_global;
    unsigned char *sbuf, *mbuf;
    unsigned start, end, len, nalloc, sz, etime, wait_time;

    pce_rwlock_rdacquire(&sp->qlock);

    if (mp->type == SG_AUDIO_OFFLINE) {
        if ((int) (sp->wtime - sp->ctime) > 0)
            etime = sp->ctime;
        else
            etime = sp->wtime;
        wait_time = mp->timeref +
            (1000 * (2 * mp->abufsize - mp->timesamp[0])) / mp->samplerate;
        if ((int) (etime - wait_time) <= 0) {
            sp->mix[mp->index].is_waiting = 1;
            sp->mix[mp->index].wait_time = wait_time;
            pce_rwlock_rdrelease(&sp->qlock);
            pce_evt_wait(&sp->mix[mp->index].evt);
            pce_rwlock_rdacquire(&sp->qlock);
            sp->mix[mp->index].is_waiting = 0;
        }
    }

    sz = sp->bufsize;
    sbuf = sp->buf;
    start = sp->mix[mp->index].pos;
    end = sp->bufcpos;
    len = (end - start) & (sz - 1);
    if (len > mp->mbufalloc - mp->mbufsize) {
        if (len > SG_AUDIO_MIXMAXBUF - mp->mbufsize)
            goto fail;
        nalloc = pce_round_up_pow2(len + mp->mbufsize);
        mbuf = realloc(mp->mbuf, nalloc);
        if (!mbuf)
            goto fail;
        mp->mbuf = mbuf;
        mp->mbufalloc = nalloc;
    }
    mbuf = mp->mbuf;
    if (end < start && end) {
        memcpy(mbuf + mp->mbufsize, sbuf + start, sz - start);
        memcpy(mbuf + mp->mbufsize + (sz - start), sbuf, end);
    } else {
        memcpy(mbuf + mp->mbufsize, sbuf + start, len);
    }
    mp->mbufsize += len;
    mp->advance = len;
    mp->wtime = sp->wtime;
    mp->ctime = sp->ctime;

    pce_rwlock_rdrelease(&sp->qlock);
    return;

fail:
    sg_logs(sg_audio_system_global.log, LOG_ERROR,
            "message queue overflow");
    abort();
}

/* Update the mixdown read position.  */
static void
sg_audio_mixdown_updatepos(struct sg_audio_mixdown *SG_RESTRICT mp)
{
    struct sg_audio_system *SG_RESTRICT sp = &sg_audio_system_global;

    if (!mp->advance)
        return;

    pce_rwlock_rdacquire(&sp->qlock);
    sp->mix[mp->index].pos =
        (sp->mix[mp->index].pos + mp->advance) & (sp->bufsize - 1);
    pce_rwlock_rdrelease(&sp->qlock);
}

/* ========================================
   Parameter automation
   ======================================== */

#define S2C(x) (((x) + (1 << SG_AUDIO_PARAMBITS) - 1) >> SG_AUDIO_PARAMBITS)

/*
  Fill a paremeter sample buffer to the given audio sample position.
  All samples strictly before the given position will be filled in.
  Note: all sample positions are audio sample positions, parameter
  sample positions are only used internally.  This may also update the
  parameter data.
*/
static void
sg_audio_mixdown_paramfill(struct sg_audio_mixdown *SG_RESTRICT mp,
                           int chan, int param, int sample)
{
    float *SG_RESTRICT pbuf;
    struct sg_audio_mixparam *SG_RESTRICT pp;
    int bufsz, pbufsz, bufnum, ca, cb, cc, ci;
    float y0, y1, dy, y;

    pp = &mp->chans[chan].params[param];

    if (sample <= pp->pos[0])
        return;

    bufsz = mp->abufsize;
    pbufsz = bufsz >> SG_AUDIO_PARAMBITS;
    bufnum = chan * SG_AUDIO_PARAMCOUNT + param;
    pbuf = mp->buf_param + pbufsz * bufnum;

    if (sample > bufsz)
        sample = bufsz;

    /* ca: first parameter sample in the linear section */
    ca = S2C(pp->pos[0]);
    /* cb: first parameter sample in the constant section */
    cb = S2C(pp->pos[1]);
    /* cc: first parameter sample beyond all sections */
    cc = S2C(sample);

    assert(ca >= 0 && cc <= pbufsz);

    if (cc < cb)
        cb = cc;

    y0 = pp->val[0];
    y1 = pp->val[1];

    if (pp->pos[1] > pp->pos[0]) {
        dy = (y1 - y0) / (pp->pos[1] - pp->pos[0]);
        y = y0 + (pp->pos[0] - ca * SG_AUDIO_PARAMRATE) * dy;
        for (ci = ca; ci < cb; ++ci)
            pbuf[ci] = y + (ci - cb) * SG_AUDIO_PARAMRATE * dy;
    } else {
        dy = 0.0f; /* to quash warning */
    }
    for (ci = cb; ci < cc; ++ci)
        pbuf[ci] = y1;

    if (sample < pp->pos[1]) {
        y = y0 + (sample - pp->pos[0]) * dy;
        pp->pos[0] = sample;
        pp->val[0] = y;
    } else {
        pp->pos[0] = sample;
        pp->pos[1] = sample;
        pp->val[0] = y1;
        pp->val[1] = y1;
    }
}

#undef S2C

/* ========================================
   Commands & message processing
   ======================================== */

static void
sg_audio_mixdown_srcstop(struct sg_audio_mixdown *SG_RESTRICT mp,
                         int src, int sample);

static void
sg_audio_mixdown_srcplay(struct sg_audio_mixdown *SG_RESTRICT mp,
                         int src, int sample,
                         const struct sg_audio_msgplay *SG_RESTRICT mplay)
{
    int chan, param;
    struct sg_audio_mixchan *chanp;
    struct sg_audio_mixparam *pp;

    if ((unsigned) src >= mp->srccount)
        return;
    sg_audio_mixdown_srcstop(mp, src, sample);
    chan = mp->chanfree;
    if (chan < 0) {
        sg_logs(sg_audio_system_global.log, LOG_WARN,
                "sound dropped");
        return;
    }
    assert((unsigned) chan < mp->chancount);
    mp->srcs[src].chan = chan;
    chanp = &mp->chans[chan];
    mp->chanfree = chanp->src;

    chanp->flags = (mplay->flags & 0xffff) | SG_AUDIO_OPEN;
    chanp->src = src;
    /* FIXME: retain, once thread safe to do so */
    chanp->sample = mplay->sample;
    chanp->pos = sample;

    for (param = 0; param < SG_AUDIO_PARAMCOUNT; ++param) {
        pp = &chanp->params[param];
        pp->pos[0] = pp->pos[1] = 0;
        pp->val[0] = pp->val[1] = mp->srcs[src].params[param];
    }
}

static void
sg_audio_mixdown_srcstop(struct sg_audio_mixdown *SG_RESTRICT mp,
                         int src, int sample)
{
    int chan, fadesamp;
    struct sg_audio_mixchan *chanp;
    struct sg_audio_mixparam *pp;
    float vol, time;

    if ((unsigned) src >= mp->srccount)
        return;
    chan = mp->srcs[src].chan;
    mp->srcs[src].chan = -1;
    if (chan < 0)
        return; /* Already stopped */
    assert((unsigned) chan < mp->chancount);
    chanp = &mp->chans[chan];
    chanp->src = -1;

    sg_audio_mixdown_paramfill(mp, chan, SG_AUDIO_VOL, sample);
    pp = &chanp->params[SG_AUDIO_VOL];
    vol = pp->val[0];
    time = (vol - SG_AUDIO_SILENT) / SG_AUDIO_FADERATE;
    if (!(time >= 0.0f))
        time = 0.0f;
    else if (!(time <= SG_AUDIO_FADETIME))
        time = SG_AUDIO_FADETIME;
    fadesamp = (int) (time * mp->samplerate);
    pp->pos[1] = pp->pos[0] + fadesamp;
    pp->val[1] = SG_AUDIO_SILENT;

    if (0) {
        printf("FADE (%d, %f) -> (%d, %f)\n",
               pp->pos[0], pp->val[0], pp->pos[1], pp->val[1]);
    }
}

static void
sg_audio_mixdown_srcreset(struct sg_audio_mixdown *SG_RESTRICT mp,
                          int src, int sample)
{
    int chan, i;
    struct sg_audio_mixchan *chanp;

    if ((unsigned) src >= mp->srccount)
        return;
    chan = mp->srcs[src].chan;
    mp->srcs[src].chan = -1;
    if (chan < 0)
        return;
    assert((unsigned) chan < mp->chancount);
    chanp = &mp->chans[chan];
    chanp->src = -1;

    for (i = 0; i < SG_AUDIO_PARAMCOUNT; ++i)
        mp->srcs[src].params[i] = 0.0f;

    (void) sample;
}

static void
sg_audio_mixdown_srcstoploop(struct sg_audio_mixdown *SG_RESTRICT mp,
                             int src, int sample)
{
    (void) mp;
    (void) src;
    (void) sample;
}

static void
sg_audio_mixdown_srcparam(struct sg_audio_mixdown *SG_RESTRICT mp,
                          int src, int sample,
                          const struct sg_audio_msgparam *SG_RESTRICT pe)
{
    int chan, dt;
    double dtf, rate;
    struct sg_audio_mixchan *chanp;
    struct sg_audio_mixparam *pp;

    if ((unsigned) src >= mp->srccount)
        return;
    mp->srcs[src].params[pe->param] = pe->val;
    chan = mp->srcs[src].chan;
    if (chan < 0)
        return;
    assert((unsigned) chan < mp->chancount);
    chanp = &mp->chans[chan];

    rate = 0.001 * mp->samplerate;
    sg_audio_mixdown_paramfill(mp, chan, pe->param, sample);
    pp = &chanp->params[pe->param];
    switch (pe->type) {
    case SG_AUDIO_PSET:
        pp->pos[1] = pp->pos[0];
        pp->val[0] = pe->val;
        pp->val[1] = pe->val;
        break;

    case SG_AUDIO_PLINEAR:
        dt = pe->d.ptime;
        if (dt < 0)
            dt = 0;
        else if (dt > SG_AUDIO_MAXPTIME)
            dt = SG_AUDIO_MAXPTIME;
        dt = (int) (rate * dt);
        pp->pos[1] = pp->pos[0] + dt;
        pp->val[1] = pe->val;
        break;

    case SG_AUDIO_PSLOPE:
        dtf = fabs((pe->val - pp->val[0]) / (pe->d.pslope * 0.001));
        if (!(dtf >= 0.0f)) {
            dt = 0;
        } else {
            if (!(dtf <= SG_AUDIO_MAXPTIME))
                dtf = SG_AUDIO_MAXPTIME;
            dt = (int) (dtf * rate);
        }
        pp->pos[1] = pp->pos[0] + dt;
        pp->val[1] = pe->val;
        break;
    }
}

static const unsigned
SG_AUDIO_MIXDOWN_MSGLEN[SG_AUDIO_MSG_COUNT] = {
    sizeof(struct sg_audio_msgplay),
    0,
    0,
    0,
    sizeof(struct sg_audio_msgparam)
};

/*
  Process current messages in the queue.  All messages with timestamps
  before the end-of-buffer timestamp are processed, messages with
  later timestamps are kept in the queue.
*/
static void
sg_audio_mixdown_dispatch(struct sg_audio_mixdown *SG_RESTRICT mp)
{
    unsigned char *mbuf;
    unsigned pos, end, type, wpos;
    unsigned hsz = pce_align(sizeof(struct sg_audio_msghdr)), msz;
    int sample;
    const struct sg_audio_msghdr *SG_RESTRICT mh;
    const void *mdat;

    mbuf = mp->mbuf;
    end = mp->mbufsize;
    wpos = pos = 0;
    while (hsz <= end - pos) {
        mh = (void *) (mbuf + pos);
        type = mh->type;
        assert(type < SG_AUDIO_MSG_COUNT);
        msz = hsz + pce_align(SG_AUDIO_MIXDOWN_MSGLEN[type]);
        assert(msz <= end - pos);
        sample = sg_audio_mixdown_t2s(mp, mh->time);
        if (sample >= mp->abufsize) {
            memmove(mbuf + wpos, mbuf + pos, msz);
            wpos += msz;
        } else {
            mdat = mbuf + pos + hsz;
            switch (type) {
            case SG_AUDIO_MSG_PLAY:
                sg_audio_mixdown_srcplay(mp, mh->src, sample, mdat);
                break;
            case SG_AUDIO_MSG_STOP:
                sg_audio_mixdown_srcstop(mp, mh->src, sample);
                break;
            case SG_AUDIO_MSG_RESET:
                sg_audio_mixdown_srcreset(mp, mh->src, sample);
                break;
            case SG_AUDIO_MSG_STOPLOOP:
                sg_audio_mixdown_srcstoploop(mp, mh->src, sample);
                break;
            case SG_AUDIO_MSG_PARAM:
                sg_audio_mixdown_srcparam(mp, mh->src, sample, mdat);
                break;
            }
        }
        pos += msz;
    }
    mp->mbufsize = wpos;
}

/* ========================================
   Constructors / destructors
   ======================================== */

static struct sg_audio_mixdown *
sg_audio_mixdown_new1(sg_audio_mixdowntype_t type,
                      int rate, int bufsize, struct sg_error **err)
{
    struct sg_audio_system *SG_RESTRICT sp = &sg_audio_system_global;
    struct sg_audio_mixdown *SG_RESTRICT mp;
    struct sg_audio_mixsrc *SG_RESTRICT srcs;
    struct sg_audio_mixchan *SG_RESTRICT chans;
    unsigned i, j, nsrc, nchan, index;
    size_t bufscount, bufslen, srcoffset, chanoffset, size;
    unsigned char *root;
    float *bufs;

    if (bufsize > 0x8000 || bufsize < 32 || (bufsize & (bufsize - 1))) {
        /* FIXME: proper error */
        sg_error_nomem(err);
        return NULL;
    }

    if (type == SG_AUDIO_LIVE && (rate < 11025 || rate > 192000)) {
        /* FIXME: proper error */
        sg_error_nomem(err);
        return NULL;
    }

    nsrc = SG_AUDIO_MAXSOURCE;
    nchan = SG_AUDIO_MAXCHAN;
    assert(nsrc > 0 && nchan > 0);

    bufscount = 4 * bufsize +
        nchan * SG_AUDIO_PARAMCOUNT * (bufsize >> SG_AUDIO_PARAMBITS);
    bufslen = pce_align(sizeof(float) * bufscount);
    srcoffset = bufslen + pce_align(sizeof(*mp));
    chanoffset = srcoffset + pce_align(sizeof(*mp->srcs) * nsrc);
    size = chanoffset + pce_align(sizeof(*mp->chans) * nchan);

    root = malloc(size);
    if (!root) {
        sg_error_nomem(err);
        return NULL;
    }
    bufs = (void *) root;
    mp = (void *) (root + bufslen);
    srcs = (void *) (root + srcoffset);
    chans = (void *) (root + chanoffset);

    for (i = 0; i < nsrc; ++i) {
        srcs[i].chan = -1;
        for (j = 0; j < SG_AUDIO_PARAMCOUNT; ++j)
            srcs[i].params[j] = 0.0f;
    }

    for (i = 0; i < nchan; ++i) {
        chans[i].flags = 0;
        chans[i].src = i + 1;
    }
    chans[nchan-1].src = -1;

    mp->root = root;

    mp->type = type;
    mp->samplerate = rate;
    mp->abufsize = bufsize;
    mp->mixahead = bufsize / 2; /* FIXME: make this configurable */
    mp->index = -1;

    mp->mbuf = NULL;
    mp->mbufsize = 0;
    mp->mbufalloc = 0;
    mp->advance = 0;

    mp->wtime = 0;
    mp->ctime = 0;
    mp->timeref = 0;
    mp->tm_n = -1;

    mp->srcs = srcs;
    mp->srccount = nsrc;
    mp->chans = chans;
    mp->chancount = nchan;
    mp->chanfree = 0;

    mp->buf_mix = bufs;
    mp->buf_samp = bufs + 2 * bufsize;
    mp->buf_param = bufs + 4 * bufsize;

    pce_lock_acquire(&sp->slock);

    if (type == SG_AUDIO_OFFLINE) {
        rate = sp->samplerate;
        if (!rate)
            rate = 48000;
        mp->samplerate = rate;
    } else if (rate != sp->samplerate) {
        if (sp->mixmask) {
            sg_logf(sp->log, LOG_ERROR,
                    "tried to change sample rate %d -> %d",
                    sp->samplerate, rate);
            free(root);
            pce_lock_release(&sp->slock);
            return NULL;
        }
        /* sg_audio_sample_setrate(rate); */
        sp->samplerate = rate;
    }

    for (index = 0; index < SG_AUDIO_MAXMIX; ++index)
        if ((sp->mixmask & (1u << index)) == 0)
            break;

    if (index >= SG_AUDIO_MAXMIX) {
        pce_lock_release(&sp->slock);
        free(root);
        /* FIXME: we need ENOSPC or some equivalent */
        sg_error_nomem(err);
        return NULL;
    }

    sp->mixmask |= 1u << index;

    sp->mix[index].pos = sp->bufrpos;
    sp->mix[index].is_waiting = 0;
    sp->mix[index].wait_time = 0;
    if (type == SG_AUDIO_OFFLINE)
        pce_evt_init(&sp->mix[index].evt);
    mp->index = index;
    mp->wtime = sp->wtime;
    mp->ctime = sp->ctime;

    pce_lock_release(&sp->slock);

    return mp;
}

struct sg_audio_mixdown *
sg_audio_mixdown_new(int rate, int bufsize, struct sg_error **err)
{
    return sg_audio_mixdown_new1(SG_AUDIO_LIVE, rate, bufsize, err);
}

struct sg_audio_mixdown *
sg_audio_mixdown_newoffline(unsigned mintime, int bufsize,
                            struct sg_error **err)
{
    struct sg_audio_mixdown *mp;
    unsigned ltime;
    mp = sg_audio_mixdown_new1(SG_AUDIO_OFFLINE, -1, bufsize, err);
    if (mp) {
        if ((int) (mp->wtime - mp->ctime) > 0)
            ltime = mp->wtime;
        else
            ltime = mp->ctime;
        if ((int) (mintime - ltime) > 0)
            mp->timeref = mintime;
        else
            mp->timeref = ltime;
        mp->timesamp[0] = bufsize;
    }
    return mp;
}

void
sg_audio_mixdown_free(struct sg_audio_mixdown *SG_RESTRICT mp)
{
    struct sg_audio_system *SG_RESTRICT sp = &sg_audio_system_global;

    pce_lock_acquire(&sp->slock);
    sp->mixmask &= ~(1u << mp->index);
    if (!sp->mixmask) {
        /* sg_audio_sample_setrate(0); */
        sp->samplerate = 0;
    }
    if (mp->type == SG_AUDIO_OFFLINE) {
        pce_evt_destroy(&sp->mix[mp->index].evt);
    }
    memset(&sp->mix[mp->index], 0, sizeof(*sp->mix));
    pce_lock_release(&sp->slock);

    free(mp->mbuf);
    free(mp->root);
}

/* ========================================
   Audio processing
   ======================================== */

static void
sg_audio_mixdown_cvolpan(float *SG_RESTRICT bp0, float *SG_RESTRICT bp1,
                         int bufsz)
{
    float vol, pan, g, g0, g1;
    int i;
    for (i = 0; i < bufsz; ++i) {
        vol = bp0[i];
        pan = bp1[i];
        if (vol < 0.0f) {
            if (vol > -80.0f) {
                g = expf(vol * (logf(10) / 20));
                if (vol > -60.0f)
                    g *= 0.05f * (g + 80.0f);
            } else {
                g = 0.0f;
            }
        } else {
            g = 1.0f;
        }
        if (pan > -1.0f) {
            if (pan < 1.0f) {
                g0 = g * sqrtf(0.5f - 0.5f * pan);
                g1 = g * sqrtf(0.5f + 0.5f * pan);
            } else {
                g0 = 0.0f;
                g1 = g;
            }
        } else {
            g0 = g;
            g1 = 0.0f;
        }
        bp0[i] = g0;
        bp1[i] = g1;
    }
}

static void
sg_audio_mixdown_render(struct sg_audio_mixdown *SG_RESTRICT mp,
                        float *SG_RESTRICT bufout)
{
    struct sg_audio_mixchan *chans;
    struct sg_audio_pcm_obj *sp;
    unsigned i, pi, chan, param, nchan, bufsz, pbufsz;
    int pos, end, length;
    float *SG_RESTRICT bufmix, *SG_RESTRICT bufsamp;
    float *bufparam, *SG_RESTRICT bp0, *SG_RESTRICT bp1;
    float x;
    const short *SG_RESTRICT sdat;

    bufsz = mp->abufsize;
    pbufsz = bufsz >> SG_AUDIO_PARAMBITS;
    nchan = mp->chancount;
    chans = mp->chans;
    bufmix = mp->buf_mix;
    bufsamp = mp->buf_samp;
    bufparam = mp->buf_param;

    memset(bufmix, 0, sizeof(float) * 2 * bufsz);

    for (chan = 0; chan < nchan; ++chan) {
        sp = chans[chan].sample;
        if (!chans[chan].flags)
            continue;
        pos = chans[chan].pos;

        bp0 = bufparam + pbufsz * SG_AUDIO_PARAMCOUNT * chan;
        bp1 = bp0 + pbufsz;

        for (param = 0; param < SG_AUDIO_PARAMCOUNT; ++param) {
            sg_audio_mixdown_paramfill(mp, chan, param, bufsz);
            chans[chan].params[param].pos[0] -= bufsz;
            chans[chan].params[param].pos[1] -= bufsz;
        }

        sg_audio_mixdown_cvolpan(bp0, bp1, pbufsz);

        // printf("pos: %d\n", pos);
        sdat = sp->buf.data;
        length = sp->buf.nframe;
        end = pos + length;
        if ((unsigned) end > bufsz)
            end = bufsz;

        /* Fill the sample buffer */

        if (pos > 0) {
            for (i = 0; (int) i < pos; ++i) {
                bufsamp[i*2+0] = 0.0f;
                bufsamp[i*2+1] = 0.0f;
            }
        } else {
            i = 0;
        }

        if (sp->buf.nchan == 2) {
            for (; (int) i < end; ++i) {
                /* FIXME: offset not included, this is major broke */
                bufsamp[i*2+0] =
                    (1.0f/32768) * (float) sdat[(i-pos)*2+0];
                bufsamp[i*2+1] =
                    (1.0f/32768) * (float) sdat[(i-pos)*2+1];
            }
        } else {
            for (; (int) i < end; ++i) {
                x = (1.0f/32768.0f) * (float) sdat[i-pos];
                bufsamp[i*2+0] = x;
                bufsamp[i*2+1] = x;
            }
        }

        for (i = end; i < bufsz; ++i) {
            bufsamp[i*2+0] = 0.0f;
            bufsamp[i*2+1] = 0.0f;
        }

        /* Mix the sample buffer into the mix buffer */

        for (i = 0; i < bufsz; ++i) {
            pi = i >> SG_AUDIO_PARAMBITS;
            bufmix[i*2+0] += bufsamp[i*2+0] * bp0[pi];
            bufmix[i*2+1] += bufsamp[i*2+1] * bp1[pi];
        }

        pos -= bufsz;
        chans[chan].pos = pos;
        if (pos < -length) {
            if (chans[chan].src >= 0)
                mp->srcs[chans[chan].src].chan = -1;
            chans[chan].flags = 0;
            chans[chan].sample = NULL;
            chans[chan].src = mp->chanfree;
            mp->chanfree = chan;
        }

        pos += bufsz;
    }

    memcpy(bufout, bufmix, sizeof(float) * 2 * bufsz);
}

/* ========================================
   Main loop
   ======================================== */

int
sg_audio_mixdown_read(struct sg_audio_mixdown *SG_RESTRICT mp,
                      unsigned time, float *buf)
{
    sg_audio_mixdown_getmsg(mp);
    sg_audio_mixdown_updatetime(mp, time);
    if (0) {
        int x = sg_audio_mixdown_t2s(mp, mp->ctime);
        printf("ctime: sample %4d%s\n",
               x, x < mp->abufsize - 1 ? " [CLIP]" : "");
    }
    sg_audio_mixdown_dispatch(mp);
    sg_audio_mixdown_updatepos(mp);
    sg_audio_mixdown_render(mp, buf);

    // printf("%6u %6u %6d\n", time, mp->wtime, (int) (time - mp->wtime));

    return 0;
}

unsigned
sg_audio_mixdown_timestamp(struct sg_audio_mixdown *SG_RESTRICT mp)
{
    return mp->timeref +
        (1000 * (mp->abufsize - mp->timesamp[0])) / mp->samplerate;
}

int
sg_audio_mixdown_samplerate(struct sg_audio_mixdown *SG_RESTRICT mp)
{
    return mp->samplerate;
}

int
sg_audio_mixdown_abufsize(struct sg_audio_mixdown *SG_RESTRICT mp)
{
    return mp->abufsize;
}
