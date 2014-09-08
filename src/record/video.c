/* Copyright 2012-2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "sg/error.h"
#include "sg/log.h"
#include "sg/record.h"
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

/*
  There are three parts to video encoding.

  An external process, FFmpeg, performs the actual encoding.  We send
  RGB data over a pipe to the encoder process.

  An internal thread waits for frames and writes them to the pipe.
  This effectively gives us a larger buffer size than normally
  afforded to us by the pipe, which is necessary to avoid blocking: on
  Linux, the default pipe buffer is 64k, which is about 3% of a single
  frame of 720p video.  The buffer size is adjustable to 1MB, but this
  still only gives us 45% of one frame.

  The exposed video structure keeps track of which frames it wants
  next.  We don't want to make too many assumptions about the video
  source: you can either render each video frames serially, or you can
  render video at a higher rate and send a subset of frames to the
  encoder.
*/

typedef enum {
    SG_VIDEOENCODER_RUN,
    SG_VIDEOENCODER_STOPPING,
    SG_VIDEOENCODER_STOPPED,
    SG_VIDEOENCODER_FAILED
} sg_videoencoder_state_t;

struct sg_videoencoder {
    /* General information */
    struct sg_logger *log;
    int width;
    int height;

    /* Encoder process */
    pid_t encoder;
    int video_pipe;

    /* IO thread */
    int has_thread;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    void *frame[SG_VIDEOENCODER_NFRAME];
    int framecount;
    sg_videoencoder_state_t state;
};

int
sg_videoencoder_timestamp(struct sg_videoencoder *rp, unsigned *time)
{
    unsigned ftime;
    int delta, halfframe;

    ftime = rp->ref_time +
        ((2 * rp->frame_num + 1) * rp->rate_denom) /
        (2 * rp->rate_numer);
    delta = *time - ftime;
    halfframe = rp->rate_denom / (2 * rp->rate_numer);

    *time = ftime;

    if (delta >= 0)
        return 2;
    if (delta >= -halfframe)
        return 1;
    return 0;
}

void
sg_videoencoder_next(struct sg_videoencoder *rp)
{
}
