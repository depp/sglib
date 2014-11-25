/* Copyright 2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "time.h"

void
sg_mixer_timeexact_init(struct sg_mixer_timeexact *mtime,
                        int bufsize, int samplerate, double reftime)
{
    mtime->bufsize = bufsize;
    mtime->m = samplerate;
    mtime->y0 = bufsize - samplerate * reftime;
}

void
sg_mixer_timeexact_update(struct sg_mixer_timeexact *mtime)
{
    mtime->y0 += mtime->bufsize;
}

int
sg_mixer_timeexact_get(struct sg_mixer_timeexact *mtime, double time)
{
    double result = time * mtime->m + mtime->y0;
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
