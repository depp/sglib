/* Copyright 2012-2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "videotime.h"

void
sg_videotime_init(struct sg_videotime *tp, unsigned time,
                  int rate_numer, int rate_denom)
{
    tp->ref_time = time;
    tp->frame_num = 0;
    tp->rate_numer = rate_numer;
    tp->rate_denom = rate_denom;
}

unsigned
sg_videotime_time(struct sg_videotime *tp)
{
    return tp->ref_time +
        ((2 * tp->frame_num + 1) * tp->rate_denom) /
        (2 * tp->rate_numer);
}

void
sg_videotime_next(struct sg_videotime *tp)
{
    tp->frame_num++;
    if (tp->frame_num >= tp->rate_numer) {
        tp->frame_num -= tp->rate_numer;
        tp->ref_time += tp->rate_denom;
    }
}
