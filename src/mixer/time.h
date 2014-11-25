/* Copyright 2013-2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "sg/defs.h"

/* A note about timestamps: The input timestamps are floating point
   seconds, and the output timestamps are signed integer sample
   positions.  Input timestamps are absolute, output timestamps are
   relative to the current buffer.  */

enum {
    /* Number of sample points used to predict latency.  A typical
       buffer size is 1024, so this uses just under 1 second of
       data at 44.1 kHz.  This must be a power of 2.  */
    SG_MIXER_TIME_NSAMP = 32
};

/* Mixer timing state.  This is used to adjust command timestamps for
   live mixers.  */
struct sg_mixer_time {
    /* (Public) Audio sample rate, in Hz.  */
    int samplerate;

    /* (Public) Audio buffer size, in samples.  Do not modify this
       value after calling update().  */
    int bufsize;

    /* (Public) The distance between points in the function mapping
       input timestamps to output timestamps, in seconds.  */
    double deltatime;

    /* (Public) Fudge factor to add to mixahead.  */
    double mixahead;

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

    /* The actual timestamps of the beginning and end of the current
       buffer, and the ratio bufsize / (buftime[1] - buftime[0]).  */
    double buftime[2];
    double buftime_m;

    /* Map from input to output timestamps.  The function is defined
       as f(t) = (t - out_x0) * out_m[i] + out_y0, where i = 0 when t
       < outx0 and i = 1 when t >= outx0.  */
    double out_x0, out_m[2], out_y0;

    /* Past commit times.  */
    double commit_sample[SG_MIXER_TIME_NSAMP];
    int commit_sample_num;
};

/* Initialize the timing system.  This uses default values for public
   parameters, which can be adjusted if you're insane enough to
   care.  */
void
sg_mixer_time_init(struct sg_mixer_time *SG_RESTRICT mtime,
                   int samplerate, int bufsize);

/* Update the state for the next buffer.  The commit time is the most
   recent commit time.  The buffer time is the timestamp corresponding
   to the end of the new buffer.  This must be called at least once
   before sg_mixer_time_get() is called.  */
void
sg_mixer_time_update(struct sg_mixer_time *SG_RESTRICT mtime,
                     double committime, double buffertime);

/* Get the sample position for the given timestamp.  Returns the
   buffer size if the sample position is after the current buffer.  */
int
sg_mixer_time_get(struct sg_mixer_time const *SG_RESTRICT mtime,
                  double time);

/* Get the current mixer latency, in samples.  */
double
sg_mixer_time_latency(struct sg_mixer_time const *SG_RESTRICT mtime);

/* Mixer timing for an offline mixer.  */
struct sg_mixer_timeexact {
    /* The audio buffer size, in samples.  */
    int bufsize;

    /* Function mapping input to output timestamps.  The function is
       defined as f(t) = t * m + y0.  */
    double m, y0;
};

/* Initialize the timing system.  */
void
sg_mixer_timeexact_init(struct sg_mixer_timeexact *mtime,
                        int bufsize, int samplerate, double reftime);

/* Update the state for the next buffer.  */
void
sg_mixer_timeexact_update(struct sg_mixer_timeexact *mtime);

/* Get the sample position for the given timestamp.  */
int
sg_mixer_timeexact_get(struct sg_mixer_timeexact *mtime, double time);
