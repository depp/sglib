/* Copyright 2012-2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include <sys/types.h>
struct sg_error;

/* Maximum number of pending frames.  */
#define SG_VIDEOIO_NFRAME 8

typedef enum {
    SG_VIDEOIO_RUN,
    SG_VIDEOIO_STOPPING,
    SG_VIDEOIO_STOPPED,
    SG_VIDEOIO_FAILED
} sg_videoio_state_t;

/* Video I/O handle */
struct sg_videoio {
    struct sg_logger *log;

    size_t framebytes;
    int pipe;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    void *frame[SG_VIDEOIO_NFRAME];
    int framecount;
    sg_videoio_state_t state;
};

/* Initialize a video I/O handle */
int
sg_videoio_init(struct sg_videoio *iop, size_t framebytes,
                int video_pipe, struct sg_error **err);

/* Destroy a video I/O handle.  Returns zero if I/O finished cleanly,
   nonzero otherwise.  */
int
sg_videoio_destroy(struct sg_videoio *iop);

/* Shutdown I/O after all buffers are flushed.  */
void
sg_videoio_stop(struct sg_videoio *iop);

/* Write a frame of video.  This takes ownership of the video frame,
   and will eventually free it with free().  This function will block
   if the buffer is full.  */
int
sg_videoio_write(struct sg_videoio *iop, void *frame);

/* Test if video I/O is running.  Returns zero if I/O has stopped,
   nonzero if I/O is running.  */
int
sg_videoio_poll(struct sg_videoio *iop);
