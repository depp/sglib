#include "audio_file.h"
#include "audio_source.h"
#include "audio_sysprivate.h"
#include "log.h"
#include "util.h"
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
    struct sg_audio_system *restrict sp = &sg_audio_system_global;
    struct sg_audio_source *srcp;
    unsigned alloc, nalloc, i, time;
    int src;

    sg_lock_acquire(&sp->slock);

    time = sg_audio_mintime(sp->wtime, sp->ctime) - 1;

    src = sp->srcfree;
    if (src == -1) {
        if (sp->srccount >= SG_AUDIO_MAXSOURCE) {
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
            srcp[i].refcount = 0;
            srcp[i].flags = i + 1;
        }
        srcp[nalloc - 1].start_time = (unsigned) -1;
        src = alloc;
    }

    srcp = &sp->srcs[src];
    sp->srcfree = srcp->flags;
    sp->srccount += 1;

    srcp->refcount = 1;
    srcp->flags = SG_AUDIO_OPEN;
    srcp->file = NULL;
    srcp->start_time = 0;
    srcp->length = 0;
    for (i = 0; i < SG_AUDIO_PARAMCOUNT; ++i) {
        srcp->param[i].time[0] = time;
        srcp->param[i].time[1] = time;
        srcp->param[i].val[0] = 0.0f;
        srcp->param[i].val[1] = 0.0f;
    }

done:
    sg_lock_release(&sp->slock);
    return src;
}

/* Destroy an audio source, adding it to the freelist.  This should
   only be called once the refcount reaches zero.  */
static void
sg_audio_source_destroy(struct sg_audio_system *restrict sp, int src)
{
    struct sg_audio_source *srcp;
    srcp = &sp->srcs[src];
    assert(srcp->refcount == 0);
    assert(srcp->file == NULL);
    srcp->refcount = 0;
    srcp->flags = sp->srcfree;
    srcp->file = NULL;
    sp->srcfree = src;
    sp->srccount -= 1;
}

void
sg_audio_source_close(int src)
{
    struct sg_audio_system *restrict sp = &sg_audio_system_global;
    struct sg_audio_source *srcp;

    sg_lock_acquire(&sp->slock);

    if ((unsigned) src >= sp->srcalloc)
        goto done;
    srcp = &sp->srcs[src];
    if (!(srcp->flags & SG_AUDIO_OPEN))
        goto done;

    srcp->flags &= ~SG_AUDIO_OPEN;
    srcp->refcount -= 1;
    if (!srcp->refcount)
        sg_audio_source_destroy(sp, src);

done:
    sg_lock_release(&sp->slock);
}

static void
sg_audio_sysmsg(struct sg_audio_system *restrict sp,
                int type, int src, int time,
                const void *restrict data, size_t len)
{
    unsigned wpos, rpos, sz, nsz, amt, i, a;
    size_t alen1, alen2, alen;
    struct sg_audio_msghdr hdr;
    unsigned char *buf, *nbuf;
    const unsigned char *ip;

    if (len > SG_AUDIO_MAXMSGLEN)
        goto fail;

    alen1 = sg_align(sizeof(hdr));
    alen2 = sg_align(len);

    wpos = sp->bufwpos;
    rpos = sp->bufrpos;
    sz = sp->bufsize;
    amt = (wpos - rpos) & (sz - 1);
    buf = sp->buf;
    if (sz - amt < alen1 + alen2) {
        nsz = sg_round_up_pow2(amt + alen1 + alen2);
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
        sg_rwlock_wracquire(&sp->qlock);
        sp->buf = nbuf;
        sp->bufsize = nsz;
        sp->bufwpos = (wpos - rpos) & (sz - 1);
        sp->bufcpos = (sp->bufcpos - rpos) & (sz - 1);
        sp->bufrpos = 0;
        for (i = 0; i < SG_AUDIO_MAXMIX; ++i)
            if (sp->mix[i].pos != (unsigned) -1)
                sp->mix[i].pos = (sp->mix[i].pos - rpos) & (sz - 1);
        sg_rwlock_wrrelease(&sp->qlock);

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

static unsigned
sg_audio_source_cliptime(struct sg_audio_system *restrict sp,
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
sg_audio_source_play(int src, unsigned time, struct sg_audio_file *file,
                     int flags)
{
    struct sg_audio_system *restrict sp = &sg_audio_system_global;
    struct sg_audio_source *srcp;
    struct sg_audio_msgplay mdat;

    sg_lock_acquire(&sp->slock);

    time = sg_audio_source_cliptime(sp, time);

    if ((unsigned) src >= sp->srcalloc)
        goto done;
    srcp = &sp->srcs[src];
    if (!(srcp->flags & SG_AUDIO_OPEN))
        goto done;

    sg_resource_incref(&file->r);
    if (srcp->file)
        sg_resource_decref(&srcp->file->r);
    else
        srcp->refcount += 1;
    srcp->file = file;
    srcp->start_time = time;
    srcp->length = 1000; /* FIXME: get actual length */

    mdat.flags = flags;
    mdat.file = file;

    sg_audio_sysmsg(sp, SG_AUDIO_MSG_PLAY, src, time, &mdat, sizeof(mdat));
    sg_resource_incref(&file->r);

done:
    sg_lock_release(&sp->slock);
}

void
sg_audio_source_stop(int src, unsigned time)
{
    struct sg_audio_system *restrict sp = &sg_audio_system_global;
    struct sg_audio_source *srcp;

    sg_lock_acquire(&sp->slock);

    time = sg_audio_source_cliptime(sp, time);

    if ((unsigned) src >= sp->srcalloc)
        goto done;
    srcp = &sp->srcs[src];
    if (!(srcp->flags & SG_AUDIO_OPEN))
        goto done;

    if (srcp->file) {
        sg_resource_decref(&srcp->file->r);
        srcp->refcount -= 1;
    }
    srcp->file = NULL;
    srcp->flags &= ~0xffffu;
    sg_audio_sysmsg(sp, SG_AUDIO_MSG_STOP, src, time, NULL, 0);

done:
    sg_lock_release(&sp->slock);
}

void
sg_audio_source_stoploop(int src, unsigned time)
{
    struct sg_audio_system *restrict sp = &sg_audio_system_global;
    struct sg_audio_source *srcp;

    sg_lock_acquire(&sp->slock);

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
    sg_lock_release(&sp->slock);
}

/* Parameter automation event type */
typedef enum {
    SG_AUDIO_PSET,
    SG_AUDIO_PLINEAR,
    SG_AUDIO_PSLOPE
} sg_audio_msgparamtype_t;

/* Parameter automation event */
struct sg_audio_paramevt {
    sg_audio_param_t param;
    sg_audio_msgparamtype_t type;
    /* start time */
    unsigned time;
    /* ending value */
    float val;
    union {
        /* For PLINEAR: delta time */
        int ptime;
        /* For PSLOPE: slope value, units per second */
        float pslope;
    } d;
};

/*
  Apply a parameter automation message to a parameter automation
  segment, producing the next segment of parameter automation.
*/
static void
sg_audio_source_papply(struct sg_audio_param *restrict pp,
                       const struct sg_audio_paramevt *restrict pe)
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
        dt = fabs((v2 - v1) / (pe->d.pslope * 0.001f));
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
sg_audio_source_pmsg(int src, struct sg_audio_paramevt *restrict pe)
{
    struct sg_audio_system *restrict sp = &sg_audio_system_global;
    struct sg_audio_source *srcp;
    struct sg_audio_param *curp;
    struct sg_audio_msgparam mdat;

    if ((unsigned) pe->param >= SG_AUDIO_PARAMCOUNT)
        return;

    sg_lock_acquire(&sp->slock);

    pe->time = sg_audio_source_cliptime(sp, pe->time);

    if ((unsigned) src >= sp->srcalloc)
        goto done;
    srcp = &sp->srcs[src];
    if (!(srcp->flags & SG_AUDIO_OPEN))
        goto done;

    curp = &srcp->param[pe->param];
    sg_audio_source_papply(curp, pe);
    mdat.param = pe->param;
    mdat.endtime = curp->time[1];
    mdat.v1 = curp->val[0];
    mdat.v2 = curp->val[1];
    sg_audio_sysmsg(
        sp, SG_AUDIO_MSG_PARAM, src,
        pe->time, &mdat, sizeof(mdat));

done:
    sg_lock_release(&sp->slock);
}

void
sg_audio_source_pset(int src, sg_audio_param_t param,
                     unsigned time, float value)
{
    struct sg_audio_paramevt pe;
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
    struct sg_audio_paramevt pe;
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
    struct sg_audio_paramevt pe;
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
sg_audio_source_bufcommit(struct sg_audio_system *restrict sp,
                          struct sg_audio_systemcbuf *bp,
                          unsigned wtime, unsigned ctime)
{
    unsigned i, rpos, nrpos, sz;
    int d, mind;

    if ((int) (wtime - ctime) > 0) {
        if ((int) (wtime - sp->ftime) > 0)
            sp->ftime = wtime;
    } else {
        if ((int) (ctime - sp->ftime) > 0)
            sp->ftime = ctime;
    }

    sg_rwlock_wracquire(&sp->qlock);

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
        if (sp->mix[i].pos == (unsigned) -1)
            continue;
        d = (sp->mix[i].pos - rpos) & (sz - 1);
        if (d < mind)
            mind = d;
    }
    nrpos = (rpos + mind) & (sz - 1);
    sp->bufrpos = nrpos;

    sg_rwlock_wrrelease(&sp->qlock);

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
sg_audio_source_bufread(struct sg_audio_systemcbuf *restrict bp,
                        size_t sz)
{
    size_t alen;
    unsigned avail, a;
    const void *p;
    unsigned char *tmp;

    alen = sg_align(sz);
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
sg_audio_source_bufskip(struct sg_audio_systemcbuf *restrict bp,
                        size_t sz)
{
    size_t alen;
    unsigned avail;
    alen = sg_align(sz);
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
sg_audio_source_bufprocess(struct sg_audio_system *restrict sp,
                           struct sg_audio_systemcbuf *restrict bp)
{
    const struct sg_audio_msghdr *restrict mhdr;
    const struct sg_audio_msgplay *restrict mplay;
    unsigned pos, wtime;
    int r;

    wtime = sp->wtime;
    while (1) {
        pos = bp->pos;
        mhdr = sg_audio_source_bufread(bp, sizeof(*mhdr));
        if (!mhdr)
            break;

        switch (mhdr->type) {
        case SG_AUDIO_MSG_PLAY:
            mplay = sg_audio_source_bufread(bp, sizeof(*mplay));
            assert(mplay != NULL);
            sg_resource_decref(&mplay->file->r);
            break;

        case SG_AUDIO_MSG_STOP:
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
}

/*
  Process each source to bring it to the current wall time.
*/
static void
sg_audio_source_sprocess(struct sg_audio_system *restrict sp)
{
    (void) sp;
#if 0
    struct sg_audio_source *srcs;
    unsigned i, maxsrc, wtime, etime, param;

    wtime = sp->wtime;
    maxsrc = sp->srcalloc;
    srcs = sp->srcs;
    for (i = 0; i < maxsrc; ++i) {
        if (!srcs[src].refcount)
            continue;
        if (srcs[src].file) {
            etime = srcs[src].start_time + srcs[src].length;
            if ((int) (wtime - etime) > 0) {
                sg_resource_decref(&srcs[src].file->r);
                srcs[src].file = NULL;
                srcs[src].refcount -= 1;
                if (!srcs[src].refcount) {
                    sg_audio_source_destroy(sp, src);
                    continue;
                }
            }
        }

        for (param = 0
        if ((int) (wtime
    }
#endif
}

void
sg_audio_source_commit(unsigned wtime, unsigned ctime)
{
    struct sg_audio_system *restrict sp = &sg_audio_system_global;
    struct sg_audio_systemcbuf buf;

    sg_lock_acquire(&sp->slock);
    sg_audio_source_bufcommit(sp, &buf, wtime, ctime);
    sg_audio_source_bufprocess(sp, &buf);
    sg_audio_source_sprocess(sp);
    sg_lock_release(&sp->slock);
}
