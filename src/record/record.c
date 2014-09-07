/* Copyright 2012-2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "sg/error.h"
#include "sg/log.h"
#include "sg/record.h"
#include "cmdargs.h"
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

/* Maximum number of pending frames.  */
#define SG_RECORD_NFRAME 8

typedef enum {
    SG_RECORD_RUN,
    SG_RECORD_STOPPING,
    SG_RECORD_STOPPED,
    SG_RECORD_FAILED
} sg_record_state_t;

struct sg_record {
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
    void *frame[SG_RECORD_NFRAME];
    int framecount;
    sg_record_state_t state;

    /* Frame timing */
    unsigned ref_time;
    int frame_num;
    int rate_numer;
    int rate_denom;
};

static void
sg_record_child(struct sg_cmdargs *cmd, int video_pipe)
{
    int fd, r;

    /* Note: we just count on CLOEXEC closing all file descriptors we
       don't want FFmpeg to have.  */

    if (video_pipe != 3) {
        r = dup2(video_pipe, 3);
        if (r < 0)
            abort();
    }

    fd = open("/dev/null", O_RDONLY);
    if (fd < 0)
        abort();
    if (fd != STDIN_FILENO) {
        r = dup2(fd, STDIN_FILENO);
        if (r < 0)
            abort();
        close(fd);
    }

    fd = open("/dev/null", O_WRONLY);
    if (fd < 0)
        abort();
    if (fd != STDOUT_FILENO) {
        r = dup2(fd, STDOUT_FILENO);
        if (r < 0)
            abort();
        close(fd);
    }

    execvp("ffmpeg", cmd->arg);
}

static int
sg_record_runproc(struct sg_record *rp, struct sg_error **err)
{
    struct sg_cmdargs cmd;
    int r, fdes[2];
    pid_t pid;
    fdes[0] = -1;
    fdes[1] = -1;
    sg_cmdargs_init(&cmd);

    r = pipe(fdes);
    if (r) {
        sg_error_errno(err, errno);
        goto error;
    }

    r = sg_cmdargs_push1(&cmd, "ffmpeg");
    if (r) goto nomem;
    r = sg_cmdargs_pushf(
        &cmd,
        " -f rawvideo"
        " -pix_fmt rgb24"
        " -r 30"
        " -s %dx%d"
        " -i pipe:3"
        " -codec:v x264"
        " -preset fast"
        " -crf 18",
        rp->width, rp->height);
    if (r) goto nomem;
    r = sg_cmdargs_push1(&cmd, "./capture.mp4");
    if (r) goto nomem;

    pid = fork();
    if (pid == 0) {
        sg_record_child(&cmd, fdes[0]);
        abort();
    }
    if (pid < 0) {
        sg_error_errno(err, errno);
        goto error;
    }

    close(fdes[0]);
    rp->encoder = pid;
    rp->video_pipe = fdes[1];
    sg_cmdargs_destroy(&cmd);
    return 0;

nomem:
    sg_error_nomem(err);
    goto error;

error:
    sg_cmdargs_destroy(&cmd);
    if (fdes[0] >= 0)
        close(fdes[0]);
    if (fdes[1] >= 0)
        close(fdes[1]);
    return -1;
}

static void *
sg_record_thread(void *arg)
{
    struct sg_record *rp = arg;
    int r, fdes, e, failed;
    unsigned char *buf;
    size_t pos, sz;
    ssize_t amt;
    struct sg_error *err = NULL;

    sz = (size_t) rp->width * rp->height * 3;
    fdes = rp->video_pipe;

    r = pthread_mutex_lock(&rp->mutex);
    if (r) abort();

    failed = 0;
    while (failed == 0) {
        while (rp->state == SG_RECORD_RUN && rp->framecount == 0) {
            r = pthread_cond_wait(&rp->cond, &rp->mutex);
            if (r) abort();
        }

        if (rp->framecount > 0) {
            buf = rp->frame[0];
            if (rp->framecount > 1) {
                memmove(rp->frame, rp->frame + 1,
                        sizeof(*rp->frame) * (rp->framecount - 1));
            }
            rp->framecount--;
        } else {
            break;
        }

        r = pthread_mutex_unlock(&rp->mutex);
        if (r) abort();

        pos = 0;
        while (pos < sz) {
            amt = write(fdes, buf + pos, sz - pos);
            if (amt < 0) {
                e = errno;
                if (e == EINTR)
                    continue;
                failed = 1;
                if (e == EPIPE) {
                    sg_logs(rp->log, SG_LOG_ERROR,
                            "video encoder closed pipe");
                } else {
                    sg_error_errno(&err, e);
                    sg_logerrs(rp->log, SG_LOG_ERROR, err,
                               "could not write to video encoder");
                    sg_error_clear(&err);
                }
                break;
            } else {
                pos += amt;
            }
        }

        free(buf);

        r = pthread_mutex_lock(&rp->mutex);
        if (r) abort();
    }

    rp->state = failed ? SG_RECORD_FAILED : SG_RECORD_STOPPED;

    r = pthread_mutex_unlock(&rp->mutex);
    if (r) abort();

    return NULL;
}

static int
sg_record_runthread(struct sg_record *rp, struct sg_error **err)
{
    int r;
    pthread_mutexattr_t mattr;
    pthread_attr_t attr;
    pthread_t thread;

    r = pthread_mutexattr_init(&mattr);
    if (r) goto err0;
    r = pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_NORMAL);
    if (r) goto err1;
    r = pthread_mutex_init(&rp->mutex, &mattr);
    if (r) goto err1;

    r = pthread_cond_init(&rp->cond, NULL);
    if (r) goto err2;

    r = pthread_attr_init(&attr);
    if (r) goto err3;
    r = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (r) goto err4;
    r = pthread_create(&thread, &attr, sg_record_thread, rp);
    if (r) goto err4;

    pthread_mutexattr_destroy(&mattr);
    pthread_attr_destroy(&attr);
    rp->has_thread = 1;
    return 0;

err4: pthread_attr_destroy(&attr);
err3: pthread_cond_destroy(&rp->cond);
err2: pthread_mutex_destroy(&rp->mutex);
err1: pthread_mutexattr_destroy(&mattr);
err0: sg_error_errno(err, r);
    return -1;
}

static int
sg_record_free(struct sg_record *rp, int do_kill)
{
    int i, r, e, fail = 0, status, retcode;
    pid_t rpid;
    struct sg_error *err = NULL;

    if (rp->video_pipe >= 0) {
        r = close(rp->video_pipe);
        if (r < 0) {
            fail = 1;
            sg_error_errno(&err, errno);
            sg_logerrs(rp->log, SG_LOG_ERROR, err,
                       "could not close video encoder pipe");
            sg_error_clear(&err);
        }
    }

    if (rp->encoder > 0) {
        if (do_kill) {
            r = kill(rp->encoder, SIGTERM);
            if (r) {
                e = errno;
                if (e != ESRCH) {
                    fail = 1;
                    sg_error_errno(&err, e);
                    sg_logerrs(rp->log, SG_LOG_ERROR, err,
                               "could not terminate encoder");
                    sg_error_clear(&err);
                }
            }
        }

        rpid = waitpid(rp->encoder, &status, WEXITED);
        if (rpid <= 0) {
            fail = 1;
            sg_error_errno(&err, errno);
            sg_logerrs(rp->log, SG_LOG_ERROR, err,
                       "could not wait for encoder");
            sg_error_clear(&err);
        } else if (WIFEXITED(status)) {
            retcode = WEXITSTATUS(status);
            if (retcode == 0) {
                sg_logs(rp->log, SG_LOG_INFO,
                        "video encoder completed successfully");
            } else {
                sg_logf(rp->log, SG_LOG_ERROR,
                        "video encoder failed with return code %d",
                        retcode);
            }
        } else if (WIFSIGNALED(status)) {
            fail = 1;
            sg_logf(rp->log, SG_LOG_ERROR,
                    "video encoder received signal %d",
                    WTERMSIG(status));
        } else {
            fail = 1;
            sg_logs(rp->log, SG_LOG_ERROR,
                    "unknown exit status");
        }
    }

    if (rp->has_thread) {
        r = pthread_mutex_destroy(&rp->mutex);
        if (r) abort();
        r = pthread_cond_destroy(&rp->cond);
        if (r) abort();
    }

    for (i = 0; i < rp->framecount; i++)
        free(rp->frame[i]);
    free(rp);

    return fail ? -1: 0;
}

struct sg_record *
sg_record_start(unsigned timestamp, int width, int height,
                struct sg_error **err)
{
    struct sg_record *rp;
    int r;

    rp = malloc(sizeof(*rp));
    if (!rp) {
        sg_error_nomem(err);
        return NULL;
    }

    rp->log = sg_logger_get("video");
    rp->width = width;
    rp->height = height;

    rp->encoder = -1;
    rp->video_pipe = -1;

    rp->has_thread = 0;
    rp->framecount = 0;
    rp->state = SG_RECORD_RUN;

    rp->ref_time = timestamp;
    rp->frame_num = 0;
    rp->rate_numer = 30;
    rp->rate_denom = 1000;

    r = sg_record_runproc(rp, err);
    if (r) {
        sg_record_free(rp, 1);
        return NULL;
    }

    r = sg_record_runthread(rp, err);
    if (r) {
        sg_record_free(rp, 1);
        return NULL;
    }

    return rp;
}

int
sg_record_stop(struct sg_record *rp)
{
    int r, failed;

    r = pthread_mutex_lock(&rp->mutex);
    if (r) abort();

    rp->state = SG_RECORD_STOPPING;
    while (rp->state == SG_RECORD_STOPPING) {
        r = pthread_cond_wait(&rp->cond, &rp->mutex);
        if (r) abort();
    }

    failed = rp->state < 0;

    r = pthread_mutex_unlock(&rp->mutex);
    if (r) abort();

    r = sg_record_free(rp, 0);
    return r || failed ? -1 : 0;
}

int
sg_record_timestamp(struct sg_record *rp, unsigned *time)
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
sg_record_next(struct sg_record *rp)
{
    rp->frame_num++;
    if (rp->frame_num >= rp->rate_numer) {
        rp->frame_num -= rp->rate_numer;
        rp->ref_time += rp->rate_denom;
    }
}

int
sg_record_write(struct sg_record *rp, void *ptr)
{
    int r, running;

    r = pthread_mutex_lock(&rp->mutex);
    if (r) abort();

    while (rp->framecount >= SG_RECORD_NFRAME &&
           rp->state == SG_RECORD_RUN) {
        r = pthread_cond_wait(&rp->cond, &rp->mutex);
        if (r) abort();
    }

    running = rp->state != SG_RECORD_RUN;
    if (running) {
        rp->frame[rp->framecount] = ptr;
        rp->framecount++;
        r = pthread_cond_broadcast(&rp->cond);
        if (r) abort();
    }

    r = pthread_mutex_unlock(&rp->mutex);
    if (r) abort();

    if (!running)
        free(ptr);

    return running;
}
