/* Copyright 2012-2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "videoio.h"
#include "sg/error.h"
#include "sg/log.h"
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void *
sg_videoio_thread(void *arg)
{
    struct sg_videoio *iop = arg;
    int r, fdes, e, failed;
    unsigned char *buf;
    size_t pos, sz;
    ssize_t amt;
    struct sg_error *err = NULL;

    sz = iop->framebytes;
    fdes = iop->pipe;

    r = pthread_mutex_lock(&iop->mutex);
    if (r) abort();

    failed = 0;
    while (failed == 0) {
        while (iop->state == SG_VIDEOIO_RUN && iop->framecount == 0) {
            r = pthread_cond_wait(&iop->cond, &iop->mutex);
            if (r) abort();
        }

        if (iop->framecount > 0) {
            buf = iop->frame[0];
            if (iop->framecount > 1) {
                memmove(iop->frame, iop->frame + 1,
                        sizeof(*iop->frame) * (iop->framecount - 1));
            }
            if (iop->framecount == SG_VIDEOIO_NFRAME) {
                r = pthread_cond_broadcast(&iop->cond);
                if (r) abort();
            }
            iop->framecount--;
        } else {
            break;
        }

        r = pthread_mutex_unlock(&iop->mutex);
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
                    sg_logs(SG_LOG_ERROR, "video encoder closed pipe.");
                } else {
                    sg_error_errno(&err, e);
                    sg_logerrs(SG_LOG_ERROR, err,
                               "Could not write to video encoder.");
                    sg_error_clear(&err);
                }
                break;
            } else {
                pos += amt;
            }
        }

        free(buf);

        r = pthread_mutex_lock(&iop->mutex);
        if (r) abort();
    }

    iop->state = failed ? SG_VIDEOIO_FAILED : SG_VIDEOIO_STOPPED;

    r = pthread_cond_broadcast(&iop->cond);
    if (r) abort();

    r = pthread_mutex_unlock(&iop->mutex);
    if (r) abort();

    return NULL;
}

int
sg_videoio_init(struct sg_videoio *iop, size_t framebytes,
                int video_pipe, struct sg_error **err)
{
    int r;
    pthread_mutexattr_t mattr;
    pthread_attr_t attr;
    pthread_t thread;

    iop->framebytes = framebytes;
    iop->pipe = video_pipe;
    iop->framecount = 0;
    iop->state = SG_VIDEOIO_RUN;

    r = pthread_mutexattr_init(&mattr);
    if (r) goto err0;
    r = pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_NORMAL);
    if (r) goto err1;
    r = pthread_mutex_init(&iop->mutex, &mattr);
    if (r) goto err1;

    r = pthread_cond_init(&iop->cond, NULL);
    if (r) goto err2;

    r = pthread_attr_init(&attr);
    if (r) goto err3;
    r = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (r) goto err4;
    r = pthread_create(&thread, &attr, sg_videoio_thread, iop);
    if (r) goto err4;

    pthread_mutexattr_destroy(&mattr);
    pthread_attr_destroy(&attr);
    return 0;

err4: pthread_attr_destroy(&attr);
err3: pthread_cond_destroy(&iop->cond);
err2: pthread_mutex_destroy(&iop->mutex);
err1: pthread_mutexattr_destroy(&mattr);
err0: sg_error_errno(err, r);
    return -1;
}

int
sg_videoio_destroy(struct sg_videoio *iop)
{
    int r, i, failed;

    /* Stop thread, wait for it to complete */

    r = pthread_mutex_lock(&iop->mutex);
    if (r) abort();

    if (iop->state == SG_VIDEOIO_RUN || iop->state == SG_VIDEOIO_STOPPING) {
        iop->state = SG_VIDEOIO_STOPPING;
        r = pthread_cond_broadcast(&iop->cond);
        if (r) abort();
        do {
            r = pthread_cond_wait(&iop->cond, &iop->mutex);
            if (r) abort();
        } while (iop->state == SG_VIDEOIO_STOPPING);
    }

    failed = iop->state != SG_VIDEOIO_STOPPED;

    r = pthread_mutex_unlock(&iop->mutex);
    if (r) abort();

    /* Destroy structures */

    r = pthread_mutex_destroy(&iop->mutex);
    if (r) abort();
    r = pthread_cond_destroy(&iop->cond);
    if (r) abort();

    for (i = 0; i < iop->framecount; i++)
        free(iop->frame[i]);

    return failed ? -1: 0;
}

void
sg_videoio_stop(struct sg_videoio *iop)
{
    int r;

    r = pthread_mutex_lock(&iop->mutex);
    if (r) abort();

    if (iop->state == SG_VIDEOIO_RUN)
        iop->state = SG_VIDEOIO_STOPPING;

    r = pthread_cond_broadcast(&iop->cond);
    if (r) abort();

    r = pthread_mutex_unlock(&iop->mutex);
    if (r) abort();
}

int
sg_videoio_write(struct sg_videoio *iop, void *frame)
{
    int r, running;

    r = pthread_mutex_lock(&iop->mutex);
    if (r) abort();

    while (iop->framecount >= SG_VIDEOIO_NFRAME &&
           iop->state == SG_VIDEOIO_RUN) {
        r = pthread_cond_wait(&iop->cond, &iop->mutex);
        if (r) abort();
    }

    running = iop->state == SG_VIDEOIO_RUN;
    if (running) {
        iop->frame[iop->framecount] = frame;
        iop->framecount++;
        r = pthread_cond_broadcast(&iop->cond);
        if (r) abort();
    }

    r = pthread_mutex_unlock(&iop->mutex);
    if (r) abort();

    if (!running)
        free(frame);

    return running;
}

int
sg_videoio_poll(struct sg_videoio *iop)
{
    int r, running;

    r = pthread_mutex_lock(&iop->mutex);
    if (r) abort();

    running = iop->state == SG_VIDEOIO_RUN ||
        iop->state == SG_VIDEOIO_STOPPING;

    r = pthread_mutex_unlock(&iop->mutex);
    if (r) abort();

    return running;
}
