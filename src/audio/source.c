/* Copyright 2012-2013 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "libpce/util.h"
#include "sg/audio_pcm.h"
#include "sg/audio_source.h"
#include "sg/log.h"
#include "sysprivate.h"
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* Maximum length of a single message */
#define SG_AUDIO_MAXMSGLEN 128

/* Initial size of message queue */
#define SG_AUDIO_MINBUF 128

/* Maximum size of message queue */
#define SG_AUDIO_MAXBUF 4096

static float
sg_audio_clip(float x, float minv, float maxv)
{
    return x >= minv ? (x <= maxv ? x : maxv) : minv;
}

int
sg_audio_source_open(void)
{
    struct sg_audio_system *SG_RESTRICT sp = &sg_audio_system_global;
    struct sg_audio_source *srcp;
    unsigned alloc, nalloc, i, time;
    int src;

    pce_lock_acquire(&sp->slock);

    time = sg_audio_mintime(sp->wtime, sp->ctime);

    src = sp->srcfree;
    if (src == -1) {
        if (sp->srcalloc >= SG_AUDIO_MAXSOURCE) {
            sg_logs(sp->log, LOG_ERROR,
                    "cannot create audio source: too many audio sources");
            goto done;
        }
        alloc = sp->srcalloc;
        nalloc = alloc ? 2 * alloc : 2;
        srcp = realloc(sp->srcs, sizeof(*srcp) * nalloc);
        if (!srcp) {
            sg_logs(sp->log, LOG_ERROR,
                    "cannot create audio source: out of memory");
            goto done;
        }
        sp->srcs = srcp;
        sp->srcalloc = nalloc;
        for (i = alloc; i < nalloc; ++i) {
            srcp[i].flags = 0;
            srcp[i].d.next = i + 1;
        }
        srcp[nalloc - 1].d.next = -1;
        src = alloc;
    }

    srcp = &sp->srcs[src];
    sp->srcfree = srcp->d.next;
    sp->srccount += 1;

    srcp->flags = SG_AUDIO_OPEN | SG_AUDIO_ACTIVE;
    srcp->d.a.msgtime = time;
    srcp->d.a.sample = NULL;
    srcp->d.a.start_time = 0;
    for (i = 0; i < SG_AUDIO_PARAMCOUNT; ++i) {
        srcp->d.a.params[i].time[0] = time;
        srcp->d.a.params[i].time[1] = time;
        srcp->d.a.params[i].val[0] = 0.0f;
        srcp->d.a.params[i].val[1] = 0.0f;
    }

done:
    pce_lock_release(&sp->slock);
    return src;
}

static void
sg_audio_sysmsg(struct sg_audio_system *SG_RESTRICT sp,
                int type, int src, int time,
                const void *SG_RESTRICT data, size_t len)
{
    unsigned wpos, rpos, sz, nsz, amt, i, a;
    size_t alen1, alen2, alen;
    struct sg_audio_msghdr hdr;
    unsigned char *buf, *nbuf;
    const unsigned char *ip;
    struct sg_audio_source *srcp;

    srcp = &sp->srcs[src];
    if ((int) (time - srcp->d.a.msgtime) > 0)
        srcp->d.a.msgtime = time;

    if (len > SG_AUDIO_MAXMSGLEN)
        goto fail;

    alen1 = pce_align(sizeof(hdr));
    alen2 = pce_align(len);

    wpos = sp->bufwpos;
    rpos = sp->bufrpos;
    sz = sp->bufsize;
    amt = (wpos - rpos) & (sz - 1);
    buf = sp->buf;
    if (sz - amt < alen1 + alen2) {
        nsz = pce_round_up_pow2(amt + alen1 + alen2);
        if (!nsz)
            goto toobig;
        if (nsz < SG_AUDIO_MINBUF)
            nsz = SG_AUDIO_MINBUF;
        if (nsz > SG_AUDIO_MAXBUF)
            goto toobig;
        nbuf = malloc(nsz);
        if (!nbuf)
            goto nomem;
        if (wpos && wpos < rpos) {
            memcpy(nbuf, buf + rpos, sz - rpos);
            memcpy(nbuf + (sz - rpos), buf, wpos);
        } else {
            memcpy(nbuf, buf + rpos, wpos - rpos);
        }

        /* We only have to acquire the lock if we need to resize the
           buffer, and we can wait until after we copy the data into
           the new buffer.  */
        pce_rwlock_wracquire(&sp->qlock);
        sp->buf = nbuf;
        sp->bufsize = nsz;
        sp->bufwpos = (wpos - rpos) & (sz - 1);
        sp->bufcpos = (sp->bufcpos - rpos) & (sz - 1);
        sp->bufrpos = 0;
        for (i = 0; i < SG_AUDIO_MAXMIX; ++i)
            if (sp->mix[i].pos != (unsigned) -1)
                sp->mix[i].pos = (sp->mix[i].pos - rpos) & (sz - 1);
        pce_rwlock_wrrelease(&sp->qlock);

        free(buf);
        buf = nbuf;
        wpos = (wpos - rpos) & (sz - 1);
        rpos = 0;
        sz = nsz;
    }

    sp->bufwpos = (wpos + alen1 + alen2) & (sz - 1);
    if ((int) (time - sp->wtime) > (int) (sp->ftime - sp->wtime))
        sp->ftime = time;

    hdr.type = (short) type;
    hdr.src = (short) src;
    hdr.time = time;
    ip = (const unsigned char *) &hdr;
    alen = alen1;
    if (alen > sz - wpos) {
        a = sz - wpos;
        memcpy(buf + wpos, ip, a);
        memcpy(buf, ip + a, alen - a);
    } else {
        memcpy(buf + wpos, ip, alen);
    }

    if (!alen2)
        return;
    wpos = (wpos + alen1) & (sz - 1);

    ip = data;
    alen = alen2;
    if (alen > sz - wpos) {
        a = sz - wpos;
        memcpy(buf + wpos, ip, a);
        memcpy(buf, ip + a, alen - a);
    } else {
        memcpy(buf + wpos, ip, alen);
    }

    return;

toobig:
    sg_logs(sp->log, LOG_ERROR, "audio message buffer full");
    goto fail;

nomem:
    sg_logs(sp->log, LOG_ERROR, "audio message buffer: out of memory");
    goto fail;

fail:
    abort();
}

void
sg_audio_source_close(int src)
{
    struct sg_audio_system *SG_RESTRICT sp = &sg_audio_system_global;
    struct sg_audio_source *srcp;

    pce_lock_acquire(&sp->slock);

    if ((unsigned) src >= sp->srcalloc)
        goto done;
    srcp = &sp->srcs[src];
    if (!(srcp->flags & SG_AUDIO_OPEN))
        goto done;

    sg_audio_sysmsg(sp, SG_AUDIO_MSG_RESET, src, srcp->d.a.msgtime,
                    NULL, 0);
    srcp->flags &= ~SG_AUDIO_OPEN;

done:
    pce_lock_release(&sp->slock);
}

static unsigned
sg_audio_source_cliptime(struct sg_audio_system *SG_RESTRICT sp,
                         unsigned time)
{
    unsigned ref = sp->wtime;
    int delta = time - ref;
    if (delta > SG_AUDIO_MAXTIME) {
        return ref + SG_AUDIO_MAXTIME;
    } else if (delta < -SG_AUDIO_MAXTIME) {
        return ref - SG_AUDIO_MAXTIME;
    } else {
        return time;
    }
}

void
sg_audio_source_play(int src, unsigned time,
                     struct sg_audio_pcm_obj *sample, int flags)
{
    struct sg_audio_system *SG_RESTRICT sp = &sg_audio_system_global;
    struct sg_audio_source *srcp;
    struct sg_audio_msgplay mdat;

    pce_lock_acquire(&sp->slock);

    time = sg_audio_source_cliptime(sp, time);

    if ((unsigned) src >= sp->srcalloc)
        goto done;
    srcp = &sp->srcs[src];
    if (!(srcp->flags & SG_AUDIO_OPEN))
        goto done;

    sg_audio_pcm_obj_incref(sample);
    if (srcp->d.a.sample)
        sg_audio_pcm_obj_decref(srcp->d.a.sample);
    srcp->d.a.sample = sample;
    srcp->d.a.start_time = time;

    mdat.flags = flags;
    mdat.sample = sample;

    sg_audio_sysmsg(sp, SG_AUDIO_MSG_PLAY, src, time, &mdat, sizeof(mdat));
    sg_audio_pcm_obj_incref(sample);

done:
    pce_lock_release(&sp->slock);
}

void
sg_audio_source_stop(int src, unsigned time)
{
    struct sg_audio_system *SG_RESTRICT sp = &sg_audio_system_global;
    struct sg_audio_source *srcp;

    pce_lock_acquire(&sp->slock);

    time = sg_audio_source_cliptime(sp, time);

    if ((unsigned) src >= sp->srcalloc)
        goto done;
    srcp = &sp->srcs[src];
    if (!(srcp->flags & SG_AUDIO_OPEN))
        goto done;

    if (srcp->d.a.sample)
        sg_audio_pcm_obj_decref(srcp->d.a.sample);
    srcp->d.a.sample = NULL;
    srcp->flags &= ~0xffffu;
    sg_audio_sysmsg(sp, SG_AUDIO_MSG_STOP, src, time, NULL, 0);

done:
    pce_lock_release(&sp->slock);
}

void
sg_audio_source_stoploop(int src, unsigned time)
{
    struct sg_audio_system *SG_RESTRICT sp = &sg_audio_system_global;
    struct sg_audio_source *srcp;

    pce_lock_acquire(&sp->slock);

    time = sg_audio_source_cliptime(sp, time);

    if ((unsigned) src >= sp->srcalloc)
        goto done;
    srcp = &sp->srcs[src];
    if (!(srcp->flags & SG_AUDIO_OPEN))
        goto done;

    if ((srcp->flags & SG_AUDIO_LOOP) != 0) {
        srcp->flags &= ~SG_AUDIO_LOOP;
        sg_audio_sysmsg(sp, SG_AUDIO_MSG_STOPLOOP, src, time, NULL, 0);
    }

done:
    pce_lock_release(&sp->slock);
}

/*
  Apply a parameter automation message to a parameter automation
  segment, producing the next segment of parameter automation.
*/
static void
sg_audio_source_papply(struct sg_audio_param *SG_RESTRICT pp,
                       const struct sg_audio_msgparam *SG_RESTRICT pe)
{
    float minv, maxv, v1, v2, dt;
    int dt1, dt2, dti;
    unsigned endt;

    /* Calculate start value */
    dt1 = pp->time[1] - pe->time;
    dt2 = pp->time[1] - pp->time[0];
    if (dt1 <= 0)
        v1 = pp->val[1];
    else if (dt1 >= dt2)
        v1 = pp->val[0];
    else
        v1 = pp->val[1] +
            ((float) dt1 / (float) dt2) * (pp->val[0] - pp->val[1]);

    /* Calculate end value */
    switch (pe->param) {
    case SG_AUDIO_VOL:
        minv = SG_AUDIO_SILENT;
        maxv = 0.0f;
        break;

    case SG_AUDIO_PAN:
        minv = -1.0f;
        maxv = +1.0f;
        break;

    default:
        goto error;
    }
    v2 = sg_audio_clip(pe->val, minv, maxv);

    /* Calculate end time */
    switch (pe->type) {
    case SG_AUDIO_PSET:
        endt = pe->time;
        break;

    case SG_AUDIO_PLINEAR:
        dti = pe->d.ptime;
        if (dti > SG_AUDIO_MAXPTIME)
            dti = SG_AUDIO_MAXPTIME;
        endt = pe->time + dti;
        break;

    case SG_AUDIO_PSLOPE:
        dt = fabsf((v2 - v1) / (pe->d.pslope * 0.001f));
        if (!(dt >= 0.0f))
            endt = pe->time;
        else if (!(dt <= SG_AUDIO_MAXPTIME))
            endt = pe->time + SG_AUDIO_MAXPTIME;
        else
            endt = pe->time + (int) floorf(dt + 0.5f);
        break;

    default:
        goto error;
    }

    pp->val[0] = v1;
    pp->val[1] = v2;
    pp->time[0] = pe->time;
    pp->time[1] = endt;

    return;

error:
    return;
}

static void
sg_audio_source_pmsg(int src, struct sg_audio_msgparam *SG_RESTRICT pe)
{
    struct sg_audio_system *SG_RESTRICT sp = &sg_audio_system_global;
    struct sg_audio_source *srcp;
    struct sg_audio_param *curp;

    if ((unsigned) pe->param >= SG_AUDIO_PARAMCOUNT)
        return;

    pce_lock_acquire(&sp->slock);

    pe->time = sg_audio_source_cliptime(sp, pe->time);

    if ((unsigned) src >= sp->srcalloc)
        goto done;
    srcp = &sp->srcs[src];
    if (!(srcp->flags & SG_AUDIO_OPEN))
        goto done;

    curp = &srcp->d.a.params[pe->param];
    sg_audio_source_papply(curp, pe);
    sg_audio_sysmsg(
        sp, SG_AUDIO_MSG_PARAM, src,
        pe->time, pe, sizeof(*pe));

done:
    pce_lock_release(&sp->slock);
}

void
sg_audio_source_pset(int src, sg_audio_param_t param,
                     unsigned time, float value)
{
    struct sg_audio_msgparam pe;
    pe.param = param;
    pe.type = SG_AUDIO_PSET;
    pe.time = time;
    pe.val = value;
    sg_audio_source_pmsg(src, &pe);
}

void
sg_audio_source_plinear(int src, sg_audio_param_t param,
                        unsigned time_start, unsigned time_end,
                        float value)
{
    struct sg_audio_msgparam pe;
    pe.param = param;
    pe.type = SG_AUDIO_PLINEAR;
    pe.time = time_start;
    pe.val = value;
    pe.d.ptime = time_end - time_start;
    sg_audio_source_pmsg(src, &pe);
}

void
sg_audio_source_pslope(int src, sg_audio_param_t param,
                       unsigned time, float value, float slope)
{
    struct sg_audio_msgparam pe;
    pe.param = param;
    pe.type = SG_AUDIO_PSLOPE;
    pe.time = time;
    pe.val = value;
    pe.d.pslope = slope;
    sg_audio_source_pmsg(src, &pe);
}

/* Temporary structure for processing mesasges */
struct sg_audio_systemcbuf {
    const unsigned char *buf;
    unsigned size;
    unsigned pos;
    unsigned end;

    union {
        void *alignment;
        unsigned char size[SG_AUDIO_MAXMSGLEN];
    } tmp;
};

/*
  Commit the time, and find the earliest buffer pointer: the location
  of the oldest message we cannot discard since it has not been
  processed by all mixdowns.  This is the only part of commit that
  requires the write lock.

  The buffer pointer is stored in the "bp" object.
*/
static void
sg_audio_source_bufcommit(struct sg_audio_system *SG_RESTRICT sp,
                          struct sg_audio_systemcbuf *bp,
                          unsigned wtime, unsigned ctime)
{
    unsigned i, rpos, nrpos, sz, etime;
    int d, mind;

    if ((int) (wtime - ctime) > 0) {
        if ((int) (wtime - sp->ftime) > 0)
            sp->ftime = wtime;
        etime = ctime;
    } else {
        if ((int) (ctime - sp->ftime) > 0)
            sp->ftime = ctime;
        etime = wtime;
    }

    pce_rwlock_wracquire(&sp->qlock);

    sp->wtime = wtime;
    sp->ctime = ctime;
    sp->bufcpos = sp->bufwpos;

    /* Find the earliest buffer pointer: the location of the oldest
       message we cannot discard, since it has not been processed by
       all mixdowns.  */
    sz = sp->bufsize;
    rpos = sp->bufrpos;
    mind = (sp->bufwpos - rpos) & (sz - 1);
    for (i = 0; i < SG_AUDIO_MAXMIX; ++i) {
        if ((sp->mixmask & (1u << i)) == 0)
            continue;
        d = (sp->mix[i].pos - rpos) & (sz - 1);
        if (d < mind)
            mind = d;
        if (sp->mix[i].is_waiting &&
            (int)(etime - sp->mix[i].wait_time) > 0)
            pce_evt_signal(&sp->mix[i].evt);
    }
    nrpos = (rpos + mind) & (sz - 1);
    sp->bufrpos = nrpos;

    pce_rwlock_wrrelease(&sp->qlock);

    bp->buf = sp->buf;
    bp->size = sp->bufsize;
    bp->pos = rpos;
    bp->end = nrpos;
}

/*
  Read from the commit buffer and advance the pointer.  Return NULL if
  not enough bytes to read.
*/
static const void *
sg_audio_source_bufread(struct sg_audio_systemcbuf *SG_RESTRICT bp,
                        size_t sz)
{
    size_t alen;
    unsigned avail, a;
    const void *p;
    unsigned char *tmp;

    alen = pce_align(sz);
    assert(sz <= SG_AUDIO_MAXMSGLEN);
    assert(alen <= SG_AUDIO_MAXMSGLEN);

    avail = (bp->end - bp->pos) & (bp->size - 1);
    if (alen > avail)
        return NULL;

    if (bp->pos < bp->end || sz < bp->size - bp->pos) {
        p = bp->buf + bp->pos;
    } else {
        p = tmp = (unsigned char *) &bp->tmp;
        a = bp->size - bp->pos;
        memcpy(tmp, bp->buf + bp->pos, a);
        memcpy(tmp + a, bp->buf, sz - a);
    }

    bp->pos = (bp->pos + alen) & (bp->size - 1);
    return p;
}

/*
  Skip over bytes in the commit buffer.  Return nonzero if successful,
  zero if not enough bytes to read.
*/
static int
sg_audio_source_bufskip(struct sg_audio_systemcbuf *SG_RESTRICT bp,
                        size_t sz)
{
    size_t alen;
    unsigned avail;
    alen = pce_align(sz);
    avail = (bp->end - bp->pos) & (bp->size - 1);
    if (alen > avail)
        return 0;
    bp->pos = (bp->pos + alen) & (bp->size - 1);
    return 1;
}

/*
  Discard all old messages, which have been processed by all the
  mixdowns.
*/
static void
sg_audio_source_bufprocess(struct sg_audio_system *SG_RESTRICT sp,
                           struct sg_audio_systemcbuf *SG_RESTRICT bp)
{
    const struct sg_audio_msghdr *SG_RESTRICT mhdr;
    const struct sg_audio_msgplay *SG_RESTRICT mplay;
    int r;

    while (1) {
        mhdr = sg_audio_source_bufread(bp, sizeof(*mhdr));
        if (!mhdr)
            break;

        switch (mhdr->type) {
        case SG_AUDIO_MSG_PLAY:
            mplay = sg_audio_source_bufread(bp, sizeof(*mplay));
            assert(mplay != NULL);
            sg_audio_pcm_obj_decref(mplay->sample);
            break;

        case SG_AUDIO_MSG_STOP:
        case SG_AUDIO_MSG_RESET:
        case SG_AUDIO_MSG_STOPLOOP:
            break;

        case SG_AUDIO_MSG_PARAM:
            r = sg_audio_source_bufskip(
                bp, sizeof(struct sg_audio_msgparam));
            assert(r != 0);
            break;

        default:
            assert(0);
            break;
        }
    }

    sp->bufrpos = bp->pos;
}

/*
  Process each source to bring it to the current wall time.
*/
static void
sg_audio_source_sprocess(struct sg_audio_system *SG_RESTRICT sp)
{
    struct sg_audio_pcm_obj *samp;
    struct sg_audio_source *SG_RESTRICT srcs;
    unsigned ctime, ltime, src;
    int i, len;

    srcs = sp->srcs;
    ctime = sp->ctime;
    ltime = sg_audio_maxtime(sp->wtime, sp->ctime);
    for (src = 0; src < sp->srcalloc; ++src) {
        if ((srcs[src].flags & (SG_AUDIO_OPEN | SG_AUDIO_ACTIVE)) == 0)
            continue;

        samp = srcs[src].d.a.sample;
        if (samp) {
            len = sg_audio_pcm_playtime(&samp->buf);
            if ((int) (ctime - srcs[src].d.a.start_time) > len) {
                /* LOOP here */
                sg_audio_pcm_obj_decref(srcs[src].d.a.sample);
                srcs[src].d.a.sample = NULL;
                srcs[src].d.a.start_time = 0;
            }
        }

        if ((srcs[src].flags & SG_AUDIO_OPEN) != 0 ||
            srcs[src].d.a.sample)
        {
            if ((int) (ltime - srcs[src].d.a.msgtime) > 0)
                srcs[src].d.a.msgtime = ltime;
        } else {
            if ((int) (ctime - srcs[src].d.a.msgtime) > 0) {
                srcs[src].flags = 0;
                srcs[src].d.next = sp->srcfree;
                sp->srcfree = src;
                sp->srccount -= 1;
                continue;
            }
        }

        for (i = 0; i < SG_AUDIO_PARAMCOUNT; ++i) {
            if ((int) (ctime - srcs[src].d.a.params[i].time[1]) > 0) {
                srcs[src].d.a.params[i].time[0] = ctime;
                srcs[src].d.a.params[i].time[1] = ctime;
                srcs[src].d.a.params[i].val[0] =
                    srcs[src].d.a.params[i].val[1];
            }
        }
    }
}

void
sg_audio_source_commit(unsigned wtime, unsigned ctime)
{
    struct sg_audio_system *SG_RESTRICT sp = &sg_audio_system_global;
    struct sg_audio_systemcbuf buf;

    pce_lock_acquire(&sp->slock);
    sg_audio_source_bufcommit(sp, &buf, wtime, ctime);
    sg_audio_source_bufprocess(sp, &buf);
    sg_audio_source_sprocess(sp);
    pce_lock_release(&sp->slock);
}
