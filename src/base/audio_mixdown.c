#include "audio_file.h"
#include "audio_mixdown.h"
#include "audio_pcm.h"
#include "audio_sysprivate.h"
#include "error.h"
#include "util.h"
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <stdio.h>

#define ALIGN(x) (((x) + sizeof(void *) - 1) & -sizeof(void *))

/* Maximum size of message queue */
#define SG_AUDIO_MIXMAXBUF 4096

struct sg_audio_mixsrc {
    int chan;
    float volume;
    float pan;
};

struct sg_audio_mixchan {
    unsigned flags;
    int src;
    struct sg_audio_file *file;

    /* The sample position of the file.  This is measured using the
       mixdown's sample rate, and is relative to the start of the
       current buffer.  */
    int pos;
};

#define SG_AUDIO_MIXDTBITS 9
#define SG_AUDIO_MIXDT (1 << (SG_AUDIO_MIXDTBITS))
#define NTIMESAMP 3

struct sg_audio_mixdown {
    /* Pointer to free in order to free the mixdown */
    void *root;

    /* ========== General info ========== */

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

    /* Map from timestamps to sample positions: the sample position,
       relative to the beginning of the current buffer, of the given
       times.  The sample in 'timestamp[0]' corresponds to the
       timestamp in 'timeref', and each next entry in the array
       corresponds to SG_AUDIO_MIXDT ticks earlier.  */
    int timesamp[NTIMESAMP];

    /* Accumulators for solving for timestamps */
    double tm_x, tm_y, tm_xx, tm_xy;
    int tm_n;

    /* Filtered (average) dt per sample */
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
};

/* ========================================
   Timing
   ======================================== */

/*
  Convert a timestamp to a sample position.
*/
static int
sg_audio_mixdown_t2s(struct sg_audio_mixdown *restrict mp,
                     unsigned time)
{
    int dt, s0, s1;

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
sg_audio_mixdown_updatetime(struct sg_audio_mixdown *restrict mp,
                            unsigned time)
{
    unsigned curtime;
    int dti, dsi, i, ns, ni;
    double ds, dt, m, b, n, s, a;

    curtime = mp->ctime;
    (void) time;

    if (mp->tm_n <= 0) {
        if (mp->tm_n < 0) {
            mp->timeref = curtime + SG_AUDIO_MIXDT;
            dsi = (SG_AUDIO_MIXDT * 0.001) * mp->samplerate;
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
sg_audio_mixdown_getmsg(struct sg_audio_mixdown *restrict mp)
{
    struct sg_audio_system *restrict sp = &sg_audio_system_global;
    unsigned char *sbuf, *mbuf;
    unsigned start, end, len, nalloc, sz;

    sg_rwlock_rdacquire(&sp->qlock);

    sz = sp->bufsize;
    sbuf = sp->buf;
    start = sp->mix[mp->index].pos;
    end = sp->bufcpos;
    len = (end - start) & (sz - 1);
    if (len > mp->mbufalloc - mp->mbufsize) {
        if (len > SG_AUDIO_MIXMAXBUF - mp->mbufsize)
            goto fail; /* FIXME: log message */
        nalloc = sg_round_up_pow2(len + mp->mbufsize);
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

    sg_rwlock_rdrelease(&sp->qlock);
    return;

fail:
    abort();
}

/* Update the mixdown read position.  */
static void
sg_audio_mixdown_updatepos(struct sg_audio_mixdown *restrict mp)
{
    struct sg_audio_system *restrict sp = &sg_audio_system_global;

    if (!mp->advance)
        return;

    sg_rwlock_rdacquire(&sp->qlock);
    sp->mix[mp->index].pos =
        (sp->mix[mp->index].pos + mp->advance) & (sp->bufsize - 1);
    sg_rwlock_rdrelease(&sp->qlock);
}

/* ========================================
   Commands & message processing
   ======================================== */

static void
sg_audio_mixdown_srcstop(struct sg_audio_mixdown *restrict mp,
                         int src, int sample);

static void
sg_audio_mixdown_srcplay(struct sg_audio_mixdown *restrict mp,
                         int src, int sample,
                         const struct sg_audio_msgplay *mplay)
{
    int chan;
    struct sg_audio_mixchan *chanp;

    if ((unsigned) src >= mp->srccount)
        return;
    sg_audio_mixdown_srcstop(mp, src, sample);
    chan = mp->chanfree;
    if (chan < 0)
        return; /* Dropped FIXME: log message */
    mp->srcs[src].chan = chan;
    chanp = &mp->chans[chan];
    mp->chanfree = chanp->src;

    chanp->flags = (mplay->flags & 0xffff) | SG_AUDIO_OPEN;
    chanp->src = src;
    /* FIXME: retain, once thread safe to do so */
    chanp->file = mplay->file;
    chanp->pos = sample;
}

static void
sg_audio_mixdown_srcstop(struct sg_audio_mixdown *restrict mp,
                         int src, int sample)
{
    int chan;
    struct sg_audio_mixchan *chanp;

    if ((unsigned) src >= mp->srccount)
        return;
    chan = mp->srcs[src].chan;
    mp->srcs[src].chan = -1;
    if (chan < 0)
        return; /* Already stopped */
    chanp = &mp->chans[chan];
    chanp->src = -1;

    (void) sample;
}

static void
sg_audio_mixdown_srcstoploop(struct sg_audio_mixdown *restrict mp,
                             int src, int sample)
{
    (void) mp;
    (void) src;
    (void) sample;
}

static void
sg_audio_mixdown_srcparam(struct sg_audio_mixdown *restrict mp,
                          int src, int sample,
                          const struct sg_audio_msgparam *mparam)
{
    (void) mp;
    (void) src;
    (void) sample;
    (void) mparam;
}

static const unsigned
SG_AUDIO_MIXDOWN_MSGLEN[SG_AUDIO_MSG_COUNT] = {
    ALIGN(sizeof(struct sg_audio_msgplay)),
    0,
    0,
    ALIGN(sizeof(struct sg_audio_msgparam))
};

/*
  Process current messages in the queue.  All messages with timestamps
  before the end-of-buffer timestamp are processed, messages with
  later timestamps are kept in the queue.
*/
static void
sg_audio_mixdown_dispatch(struct sg_audio_mixdown *restrict mp)
{
    unsigned char *mbuf;
    unsigned pos, end, type, wpos;
    unsigned hsz = ALIGN(sizeof(struct sg_audio_msghdr)), msz;
    int sample;
    const struct sg_audio_msghdr *restrict mh;
    const void *mdat;

    mbuf = mp->mbuf;
    end = mp->mbufsize;
    wpos = pos = 0;
    while (hsz <= end - pos) {
        mh = (void *) (mbuf + pos);
        type = mh->type;
        assert(type < SG_AUDIO_MSG_COUNT);
        msz = hsz + SG_AUDIO_MIXDOWN_MSGLEN[type];
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

struct sg_audio_mixdown *
sg_audio_mixdown_new(int rate, int bufsize, struct sg_error **err)
{
    struct sg_audio_system *restrict sp = &sg_audio_system_global;
    struct sg_audio_mixdown *restrict mp;
    struct sg_audio_mixsrc *restrict srcs;
    struct sg_audio_mixchan *restrict chans;
    unsigned i, nsrc, nchan, index;
    size_t bufslen, srcoffset, chanoffset, size;
    unsigned char *root;
    float *bufs;

    if (bufsize > 0x8000 || bufsize < 32 || (bufsize & (bufsize - 1))) {
        /* FIXME: proper error */
        sg_error_nomem(err);
        return NULL;
    }

    if (rate < 11025 || rate > 192000) {
        /* FIXME: proper error */
        sg_error_nomem(err);
        return NULL;
    }

    nsrc = SG_AUDIO_MAXSOURCE;
    nchan = SG_AUDIO_MAXCHAN;
    assert(nsrc > 0 && nchan > 0);

    bufslen = sg_align(sizeof(float) * 4 * bufsize);
    srcoffset = bufslen + sg_align(sizeof(*mp));
    chanoffset = srcoffset + sg_align(sizeof(*mp->srcs) * nsrc);
    size = chanoffset + sg_align(sizeof(*mp->chans) * nchan);

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
        srcs[i].volume = 0.0f;
        srcs[i].pan = 0.0f;
    }

    for (i = 0; i < nchan; ++i) {
        chans[i].flags = 0;
        chans[i].src = i + 1;
    }
    chans[nchan-1].src = -1;

    mp->root = root;

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
    mp->tm_n = -1;

    mp->srcs = srcs;
    mp->srccount = nsrc;
    mp->chans = chans;
    mp->chancount = nchan;
    mp->chanfree = 0;

    mp->buf_mix = bufs;
    mp->buf_samp = bufs + 2 * bufsize;

    sg_lock_acquire(&sp->slock);

    for (index = 0; index < SG_AUDIO_MAXMIX; ++index)
        if ((sp->mixmask & (1u << index)) == 0)
            break;

    if (index >= SG_AUDIO_MAXMIX) {
        sg_lock_release(&sp->slock);
        free(mp);
        /* FIXME: we need ENOSPC or some equivalent */
        sg_error_nomem(err);
        return NULL;
    }

    sp->mix[index].pos = sp->bufrpos;
    mp->index = index;
    mp->wtime = sp->wtime;
    mp->ctime = sp->ctime;

    sg_lock_release(&sp->slock);

    return mp;
}

void
sg_audio_mixdown_free(struct sg_audio_mixdown *restrict mp)
{
    struct sg_audio_system *restrict sp = &sg_audio_system_global;

    sg_lock_acquire(&sp->slock);
    sp->mixmask &= ~(1u << mp->index);
    sg_lock_release(&sp->slock);

    free(mp->mbuf);
    free(mp->root);
}

/* ========================================
   Audio processing
   ======================================== */

static void
sg_audio_mixdown_render(struct sg_audio_mixdown *restrict mp,
                        float *restrict bufout)
{
    struct sg_audio_mixchan *restrict chans;
    struct sg_audio_file *restrict fp;
    unsigned i, j, nchan, bufsz, rate;
    int r, pos, length;
    float *restrict bufmix, *restrict bufsamp;

    bufsz = mp->abufsize;
    nchan = mp->chancount;
    chans = mp->chans;
    rate = mp->samplerate;
    bufmix = mp->buf_mix;
    bufsamp = mp->buf_samp;

    memset(bufmix, 0, sizeof(float) * 2 * bufsz);

    for (i = 0; i < nchan; ++i) {
        fp = chans[i].file;
        if (!chans[i].flags)
            continue;
        pos = chans[i].pos;

        /* FIXME: we could actually use atomics here, maybe? */
        /* FIXME: don't use locks... */
        r = sg_lock_try(&fp->lock);
        if (r) {
            // printf("pos: %d\n", pos);
            if (fp->type == SG_AUDIO_PCM) {
                sg_audio_pcm_resample(&fp->data.pcm, bufsamp,
                                      bufsz, pos, rate);
                sg_lock_release(&fp->lock);
                for (j = 0; j < bufsz; ++j) {
                    bufmix[j*2+0] += bufsamp[j*2+0];
                    bufmix[j*2+1] += bufsamp[j*2+1];
                }
                length = (int) fp->data.pcm.nframe *
                    ((double) rate / fp->data.pcm.rate);
            } else {
                sg_lock_release(&fp->lock);
                length = rate;
            }
            pos -= bufsz;
            chans[i].pos = pos;
            if (pos < -length) {
                chans[i].flags = 0;
                chans[i].file = 0;
                chans[i].src = mp->chanfree;
                mp->chanfree = i;
            }
        }

        pos += bufsz;
    }

    memcpy(bufout, bufmix, sizeof(float) * 2 * bufsz);
}

/* ========================================
   Main loop
   ======================================== */

int
sg_audio_mixdown_read(struct sg_audio_mixdown *restrict mp,
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
