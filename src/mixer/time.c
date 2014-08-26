/* Copyright 2013-2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "time.h"
#include <math.h>

#if defined _WIN32
# include <float.h>
# ifndef isfinite
#  define isfinite _finite
# endif
#endif

/*
  There are three things that the realtime mixer tries to accomplish:

  1. Avoid putting events in the buffer before they were scheduled to
  appear.  This only happens if events are queued in advance, e.g., if
  a recording is being played back.  This is done in the get()
  function by calculating both the delayed event timestamp and the
  actual event timestamp, and choosing the later one.

  2. Avoid placing timestampse beyond the commit time in the buffer.
  This creates discontinuities in the correspondence between input and
  output timestamps.  This is done using linear predictions of the
  commit time.

  3. Avoid abrupt changes in latency.
*/

void
sg_mixer_time_init(struct sg_mixer_time *SG_RESTRICT mtime,
                   int samplerate, int bufsize)
{
    mtime->samplerate = samplerate;
    mtime->bufsize = bufsize;
    mtime->deltabits = 8;
    mtime->mixahead = bufsize >> 3;
    mtime->safety = 2.0;
    mtime->maxslope = 1.5;
    mtime->smoothing = 0.75;
    mtime->initted = 0;
}

/* Update the map from timestamps to sample positions.  */
static void
sg_mixer_time_predict(struct sg_mixer_time *SG_RESTRICT mtime)
{
    int i, idx;
    double x, t, ax, at, axx, axt, fac, pa, pb, avar, d, y, dy, maxdy;

    /* Update the samples to be relative to the next intime, advance
       the output map */
    for (i = SG_MIXER_TIME_NPOINT - 1; i > 0; i--)
        mtime->outtime[i] = mtime->outtime[i - 1];
    mtime->intime += 1 << mtime->deltabits;
    for (i = 0; i < SG_MIXER_TIME_NSAMP; i++)
        mtime->commit_sample[i] -= 1 << mtime->deltabits;

    /* Linear regression mapping commit time to buffer position,
       position = pa + pb * commit_time */
    ax = at = axx = axt = 0.0;
    for (i = 0; i < SG_MIXER_TIME_NSAMP; i++) {
        idx = (i + mtime->commit_sample_num) & (SG_MIXER_TIME_NSAMP - 1);
        t = (i + 2 - SG_MIXER_TIME_NSAMP) * mtime->bufsize;
        x = mtime->commit_sample[idx];
        ax += x;
        at += t;
        axx += x * x;
        axt += x * t;
    }
    fac = 1.0 / SG_MIXER_TIME_NSAMP;
    pb = (axt - ax * at * fac) / (axx - ax * ax * fac);
    pa = (at - pb * ax) * fac;

    /* Calculate remaining variance */
    avar = 0.0;
    for (i = 0; i < SG_MIXER_TIME_NSAMP; i++) {
        idx = (i + mtime->commit_sample_num) & (SG_MIXER_TIME_NSAMP - 1);
        t = (i + 2 - SG_MIXER_TIME_NSAMP) * mtime->bufsize;
        x = mtime->commit_sample[idx];
        d = pa + pb * x - t;
        avar += d * d;
    }
    avar *= fac;

    /* Calculate buffer position for new data point */
    y = pa + sqrt(avar) * mtime->safety + mtime->mixahead;
    if (isfinite(y)) {
        dy = y - mtime->outtime[1];
        maxdy = (mtime->samplerate << mtime->deltabits) *
            mtime->maxslope * 0.001;
        if (dy > maxdy)
            dy = maxdy;
        else if (dy < mtime->bufsize)
            dy = mtime->bufsize;
    } else {
        dy = mtime->bufsize;
    }

    /* Smooth the buffer position output with an IIR filter */
    dy += mtime->smoothing * ((mtime->outtime[1] - mtime->outtime[2]) - dy);
    mtime->outtime[0] = (int) ceil(y);
}

void
sg_mixer_time_update(struct sg_mixer_time *SG_RESTRICT mtime,
                     unsigned committime, unsigned buffertime)
{
    int i, delta;

    if (!mtime->initted) {
        mtime->initted = 1;
        mtime->buftime[0] = buffertime - mtime->bufsize;
        mtime->buftime[1] = buffertime;
        mtime->intime = committime;
        delta = (mtime->samplerate << mtime->deltabits) / 1000;
        for (i = 0; i < SG_MIXER_TIME_NPOINT; i++)
            mtime->outtime[i] = mtime->bufsize - delta * i;
        delta = (mtime->bufsize * 1000) / mtime->samplerate;
        for (i = 0; i < SG_MIXER_TIME_NSAMP; i++)
            mtime->commit_sample[i] = delta *
                (i + 1 - SG_MIXER_TIME_NSAMP);
        mtime->commit_sample_num = 0;
        return;
    }

    mtime->buftime[0] = mtime->buftime[1];
    if (mtime->buftime[0] == buffertime)
        mtime->buftime[0] = buffertime - 1;
    mtime->buftime[1] = buffertime;
    for (i = 0; i < SG_MIXER_TIME_NPOINT; i++)
        mtime->outtime[i] -= mtime->bufsize;
    mtime->commit_sample[mtime->commit_sample_num] =
        committime - mtime->intime;
    mtime->commit_sample_num = (mtime->commit_sample_num + 1) &
        (SG_MIXER_TIME_NSAMP - 1);
    if (mtime->outtime[0] < mtime->bufsize)
        sg_mixer_time_predict(mtime);
}

/* Get the delayed output timestamp for an input timestamp.  */
static int
sg_mixer_time_get_delayed(struct sg_mixer_time const *SG_RESTRICT mtime,
                          unsigned time)
{
    /* Find two timestamps with known sample positions surrounding the
       given timestamp, and interpolate between them.  If the
       timestamp is outside the timestamps with known sample times,
       then refuse to extrapolate and simply clamp the sample position
       to the nearest known point.  */
    int dt, s0, s1, delta = 1 << mtime->deltabits;
    dt = mtime->intime - time;
    if (dt < delta) {
        if (dt <= 0)
            return mtime->outtime[0];
        s0 = mtime->outtime[0];
        s1 = mtime->outtime[1];
    } else {
        dt -= delta;
        if (dt >= delta)
            return mtime->outtime[2];
        s0 = mtime->outtime[1];
        s1 = mtime->outtime[2];
    }
    return (s0 * (delta - dt) + s1 * dt) >> mtime->deltabits;
}

int
sg_mixer_time_get(struct sg_mixer_time const *SG_RESTRICT mtime,
                  unsigned time)
{
    int reltime, bufdelta, abstime, delayedtime;
    reltime = time - mtime->buftime[0];
    bufdelta = mtime->buftime[1] - mtime->buftime[0];
    if (reltime >= bufdelta)
        return mtime->bufsize;
    delayedtime = sg_mixer_time_get_delayed(mtime, time);
    abstime = reltime <= 0 ? 0 : reltime * mtime->bufsize / bufdelta;
    return abstime > delayedtime ? abstime : delayedtime;
}

double
sg_mixer_time_latency(struct sg_mixer_time const *SG_RESTRICT mtime)
{
    return mtime->buftime[1] - mtime->intime +
        (double) (mtime->buftime[1] - mtime->outtime[0]) *
        (1 << mtime->deltabits) /
        (mtime->outtime[1] - mtime->outtime[0]);
}
