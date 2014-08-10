/* Copyright 2013-2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "sg/defs.h"

/*
  A note about timestamps: The input timestamps are unsigned sample
  positions, and the output timestamps are signed sample positions.
  This is because the input timestamps can go on forever, wrapping
  around (about once per day at 48 kHz).  The output timestamps are
  relative to the current buffer.
*/

enum {
    /* Number of points used to map input to output timestamps */
    SG_MIXER_TIME_NPOINT = 3,

    /* Number of sample points used to predict latency.  A typical
       buffer size is 1024, so this uses just under 1 second of
       data at 44.1 kHz.  This must be a power of 2.  */
    SG_MIXER_TIME_NSAMP = 32
};

/* Mixer timing state.  This is used to adjust command timestamps for
   live mixers.  */
struct sg_mixer_time {
    /* (Public) Audio buffer size, in samples.  Do not modify this
       value after calling update().  */
    int bufsize;

    /* (Public) Log 2 of the distance between points in the function
       mapping input to output timestamps.  This distance should be
       larger than the buffer size, but no larger than 15.  Do not
       modify this value after calling update().  */
    int deltabits;

    /* (Public) Fudge factor to add to mixahead.  */
    int mixahead;

    /* (Public) Buffer safety margin, in standard deviations.  0
       selects a mixahead delay that puts 50% of commit times after
       the end of the buffer, 1 gives 84%, 2 gives 98%.  These
       percentages are calculated assuming that commit time deviations
       from linearity are normally distributed.  */
    double safety;

    /* (Public) The maximum slope of the map from input to output
       time.  Increasing this can cause latency spikes to have a
       longer lasting effect on delay.  Decreasing this increases the
       time required to respond to increases in latency.  The value
       should be larger than the ratio of the output clock rate
       divided by the input clock rate (nominally 1.0).  */
    double maxslope;

    /* (Public) The smoothing factor for changes in latency, between
       0.0 and 1.0.  Larger numbers apply more smoothing.  */
    double smoothing;

    /* Flag indicating the rest of the structure is initialized */
    int initted;

    /* The timestamp of the beginning and end of the current buffer.  */
    unsigned buftime[2];

    /* Reference time for input timestamps.  This will move ahead by
       TIMEDELTA as necessary to keep the output audio buffer
       timestamps within range.  */
    unsigned intime;

    /* Map from input timestamps to output timestamps.  The input
       timestamp `intime - TIMEDELTA * i` corresponds to the output
       timestamp `outtime[i]`.  This is chosen so that the first entry
       always >= bufsize.  */
    int outtime[SG_MIXER_TIME_NPOINT];

    /* Past commit times, relative to the current intime */
    int commit_sample[SG_MIXER_TIME_NSAMP];
    int commit_sample_num;
};

/* Initialize the timing system.  This uses default values for public
   parameters, which can be adjusted if you're insane enough to
   care.  */
void
sg_mixer_time_init(struct sg_mixer_time *SG_RESTRICT mtime,
                   int bufsize);

/* Update the state for the next buffer.  The commit time is the most
   recent commit time.  The buffer time is the timestamp corresponding
   to the end of the new buffer.  This must be called at least once
   before sg_mixer_time_get() is called.  */
void
sg_mixer_time_update(struct sg_mixer_time *SG_RESTRICT mtime,
                     unsigned committime, unsigned buffertime);

/* Get the sample position for the given timestamp.  Returns the
   buffer size if the sample position is after the current buffer.  */
int
sg_mixer_time_get(struct sg_mixer_time const *SG_RESTRICT mtime,
                  unsigned time);

/* Get the current mixer latency, in samples.  */
double
sg_mixer_time_latency(struct sg_mixer_time const *SG_RESTRICT mtime);
