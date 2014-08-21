/* Copyright 2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "time.h"

void
sg_mixer_timeexact_init(struct sg_mixer_timeexact *mtime,
                        int bufsize, int samplerate, unsigned reftime)
{
    mtime->bufsize = bufsize;
    mtime->samplerate = samplerate;
    mtime->intime = reftime;
    mtime->outtime = 0;
}

void
sg_mixer_timeexact_update(struct sg_mixer_timeexact *mtime)
{
    mtime->outtime -= mtime->bufsize;
    if (mtime->outtime <= -mtime->samplerate) {
        mtime->intime += 1000;
        mtime->outtime += mtime->samplerate;
    }
}

int
sg_mixer_timeexact_get(struct sg_mixer_timeexact *mtime,
                       unsigned time)
{
    int delta = (int) (time - mtime->intime);
    if (delta < 0)
        return 0;
    if (delta > 2000)
        return mtime->bufsize;
    return mtime->outtime + (delta * mtime->samplerate) / 1000;
}
