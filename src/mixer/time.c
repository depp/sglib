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
    mtime->deltatime = 0.125;
    mtime->mixahead = (double) bufsize / samplerate * 0.125;
    mtime->safety = 2.0;
    mtime->maxslope = 1.5;
    mtime->smoothing = 0.75;
    mtime->initted = 0;
}

/* Update the map from timestamps to sample positions.  */
static void
sg_mixer_time_predict(struct sg_mixer_time *SG_RESTRICT mtime)
{
    double x0, fac, pa, pb, dev;

    x0 = mtime->out_x0;
    fac = 1.0 / SG_MIXER_TIME_NSAMP;

    {
        int i, idx;
        double x, t, ax = 0.0, at = 0.0, axx = 0.0, axt = 0.0;
        /* Linear regression mapping commit time to buffer position,
           position = pa + pb * commit_time */
        for (i = 0; i < SG_MIXER_TIME_NSAMP; i++) {
            idx = (i + mtime->commit_sample_num) & (SG_MIXER_TIME_NSAMP - 1);
            t = i + 2 - SG_MIXER_TIME_NSAMP;
            x = mtime->commit_sample[idx] - x0;
            ax += x;
            at += t;
            axx += x * x;
            axt += x * t;
        }
        pb = (axt - ax * at * fac) / (axx - ax * ax * fac);
        pa = (at - pb * ax) * fac;
    }

    {
        int i, idx;
        double x, t, avar = 0.0, d;
        /* Calculate variance from fit */
        avar = 0.0;
        for (i = 0; i < SG_MIXER_TIME_NSAMP; i++) {
            idx = (i + mtime->commit_sample_num) & (SG_MIXER_TIME_NSAMP - 1);
            t = i + 2 - SG_MIXER_TIME_NSAMP;
            x = mtime->commit_sample[idx] - x0;
            d = pa + pb * x - t;
            avar += d * d;
        }
        dev = sqrt(avar * fac);
    }

    {
        double y0, y1, dy, maxdy;
        /* Calculate buffer position for new data point */
        dev = 0;
        y0 = mtime->out_m[1] * mtime->deltatime;
        y1 = (pa + pb * mtime->deltatime * 2.0 + dev * mtime->safety) *
            mtime->bufsize +
            mtime->mixahead * mtime->samplerate -
            mtime->out_y0;
        if (isfinite(y1)) {
            dy = y1 - y0;
            maxdy = mtime->samplerate * mtime->deltatime * mtime->maxslope;
            if (dy > maxdy && 0)
                dy = maxdy;
            else if (dy < mtime->bufsize)
                dy = mtime->bufsize;
        } else {
            dy = mtime->bufsize;
        }

        /* Smooth the buffer position output with an IIR filter */
        /* dy += mtime->smoothing * (y0 - dy); */

        mtime->out_x0 += mtime->deltatime;
        mtime->out_m[0] = mtime->out_m[1];
        mtime->out_m[1] = dy / mtime->deltatime;
        mtime->out_y0 += y0;
    }
}

void
sg_mixer_time_update(struct sg_mixer_time *SG_RESTRICT mtime,
                     double committime, double buffertime)
{
    if (!mtime->initted) {
        int i;
        double delta;

        delta = (double) mtime->bufsize / mtime->samplerate;
        mtime->initted = 1;
        mtime->buftime[0] = buffertime - delta;
        mtime->buftime[1] = buffertime;
        mtime->out_x0 = committime;
        mtime->out_m[0] = mtime->out_m[1] = mtime->samplerate;
        mtime->out_y0 = mtime->bufsize;
        for (i = 0; i < SG_MIXER_TIME_NSAMP; i++)
            mtime->commit_sample[i] = committime +
                delta * (i + 1 - SG_MIXER_TIME_NSAMP);
        mtime->commit_sample_num = 0;
    } else {
        double y;

        mtime->buftime[0] = mtime->buftime[1];
        mtime->buftime[1] = buffertime;
        mtime->out_y0 -= mtime->bufsize;
        mtime->commit_sample[mtime->commit_sample_num] = committime;
        mtime->commit_sample_num = (mtime->commit_sample_num + 1) &
            (SG_MIXER_TIME_NSAMP - 1);
        y = mtime->out_m[1] * mtime->deltatime + mtime->out_y0;
        if (y < mtime->bufsize)
            sg_mixer_time_predict(mtime);
    }

    mtime->buftime_m =
        (double) mtime->bufsize / (mtime->buftime[1] - mtime->buftime[0]);
}

int
sg_mixer_time_get(struct sg_mixer_time const *SG_RESTRICT mtime,
                  double time)
{
    double reltime, result;

    reltime = time - mtime->out_x0;
    result = reltime * mtime->out_m[reltime >= 0.0] + mtime->out_y0;

    reltime = time - mtime->buftime[0];
    if (reltime >= 0.0 && 0) {
        double exact;
        exact = (time - mtime->buftime[0]) * mtime->buftime_m;
        if (exact > result)
            result = exact;
    }

    if (result >= 0.0) {
        if (result <= mtime->bufsize) {
            return (int) result;
        } else {
            return mtime->bufsize;
        }
    } else {
        return 0;
    }
}

double
sg_mixer_time_latency(struct sg_mixer_time const *SG_RESTRICT mtime)
{
    return (mtime->buftime[1] - mtime->out_x0) -
        (mtime->bufsize - mtime->out_y0) / mtime->out_m[1];
}
