/* Copyright 2012-2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */

/* Video encoder frame timing */
struct sg_videotime {
    unsigned ref_time;
    int frame_num;
    int rate_numer;
    int rate_denom;
};

/* Initialize frame rate timing structure.  */
void
sg_videotime_init(struct sg_videotime *tp, unsigned time,
                  int rate_numer, int rate_denom);

/* Get the timestamp of the next frame.  */
unsigned
sg_videotime_time(struct sg_videotime *tp);

/* Advance to the next frame.  */
void
sg_videotime_next(struct sg_videotime *tp);
