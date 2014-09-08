/* Copyright 2012-2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include <sys/types.h>
struct sg_error;

/* Video encoder process */
struct sg_videoproc {
    struct sg_logger *log;

    /* Process ID of the video encoder */
    pid_t encoder;

    /* Pipe for sending video data to the encoder */
    int pipe;

    /* Indicates that the encoder did not complete successfully */
    int failed;
};

/* Initialize the video encoder, creating a pipe and running the
   process.  */
int
sg_videoproc_init(struct sg_videoproc *pp, const char *path,
                  int width, int height, struct sg_error **err);

/* Dispose of the video encoder, closing the pipe and waiting for the
   video encoder to exit.  If do_kill is set, then the video encoder
   process will be sent SIGTERM.  Returns zero if everything finished
   cleanly, nonzero otherwise.  */
int
sg_videoproc_destroy(struct sg_videoproc *pp, int do_kill);

/* Close the video encoding pipe.  */
void
sg_videoproc_close(struct sg_videoproc *pp);

/* Test to see if the encoder process is still running.  Return zero
   if the encoder has stopped, or nonzero if the encoder is
   running.  */
int
sg_videoproc_poll(struct sg_videoproc *pp);
