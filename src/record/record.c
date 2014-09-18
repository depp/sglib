/* Copyright 2012-2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "record.h"
#include "screenshot.h"
#include "videoio.h"
#include "videoparam.h"
#include "videoproc.h"
#include "videotime.h"
#include "sg/clock.h"
#include "sg/entry.h"
#include "sg/error.h"
#include "sg/log.h"
#include "sg/opengl.h"
#include "sg/record.h"
#include "../core/private.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

/* Flags for recording and buffers */
enum {
    /* The buffer contains a screenshot, or a screenshot has been
       requested */
    SG_RECORD_FRAME_SCREENSHOT = 1u << 0,

    /* The buffer contains a frame of video, or a frame of video has
       been requested */
    SG_RECORD_FRAME_VIDEO = 1u << 1,

    /* Indicates a frame has started.  */
    SG_RECORD_INFRAME = 1u << 2,

    /* Indicates which buffer will be used next */
    SG_RECORD_WHICHBUF = 1u << 3,

    /* Video recording is active */
    SG_RECORD_VIDEO = 1u << 4,

    /* Video I/O is running */
    SG_RECORD_HASVIO = 1u << 5,

    /* Video encoder process is running */
    SG_RECORD_HASVPROC = 1u << 6,

    /* Mask for pixel data requests */
    SG_RECORD_FRAMEMASK = SG_RECORD_FRAME_SCREENSHOT | SG_RECORD_FRAME_VIDEO
};

/* Image buffer for readback from the renderer */
struct sg_record_buf {
    unsigned flags;
    GLuint buf;
    int width, height;
};

struct sg_record {
    struct sg_logger *log;
    unsigned flags;
    int fwidth, fheight;
    struct sg_record_buf buf[2];

    int vwidth, vheight;
    size_t framebytes;
    struct sg_videotime vtime;
    struct sg_videoproc vproc;
    struct sg_videoio vio;
};

static struct sg_record sg_record;

static void
sg_record_buf_read(struct sg_record_buf *buf,
                   int x, int y, int width, int height,
                   unsigned flags)
{
    if (!buf->buf) {
        glGenBuffers(1, &buf->buf);
        if (!buf->buf)
            goto err;
    }

    glBindBuffer(GL_PIXEL_PACK_BUFFER, buf->buf);
    glBufferData(GL_PIXEL_PACK_BUFFER, width * height * 4,
                 NULL, GL_STREAM_READ);
    glReadPixels(x, y, width, height,
                 GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

    buf->flags = flags;
    buf->width = width;
    buf->height = height;

err:
    sg_opengl_checkerror("sg_record_buf_read");
}

static void
sg_record_stopvio(void)
{
    struct sg_record_buf *buf;
    int i;
    sg_videoio_destroy(&sg_record.vio);
    sg_videoproc_close(&sg_record.vproc);
    sg_record.flags &= ~(SG_RECORD_HASVIO | SG_RECORD_VIDEO);
    for (i = 0; i < 2; i++) {
        buf = &sg_record.buf[i];
        if (buf->buf == 0 || (buf->flags & ~SG_RECORD_FRAME_VIDEO) != 0)
            continue;
        glDeleteBuffers(1, &buf->buf);
        buf->flags = 0;
        buf->buf = 0;
        buf->width = 0;
        buf->height = 0;
    }
}

static void
sg_record_stopvproc(void)
{
    if ((sg_record.flags & SG_RECORD_HASVIO) != 0)
        sg_record_stopvio();
    sg_videoproc_destroy(&sg_record.vproc, 0);
    sg_record.flags &= ~SG_RECORD_HASVPROC;
}

static void
sg_record_writevideo(void *mptr, int width, int height)
{
    int r;

    if ((sg_record.flags & SG_RECORD_HASVIO) == 0) {
        free(mptr);
        return;
    }
    if (width != sg_record.vwidth || height != sg_record.vheight) {
        sg_logs(sg_record.log, SG_LOG_WARN,
                "framebuffer changed size, stopping video encoding");
        free(mptr);
        r = 0;
    } else {
        r = sg_videoio_write(&sg_record.vio, mptr);
    }
    if (!r)
        sg_record.flags &= ~SG_RECORD_VIDEO;
}

static void
sg_record_buf_process(struct sg_record_buf *buf)
{
    struct sg_error *err = NULL;
    void *mptr, *fptr;
    int width, height, i;
    size_t sz;

    if (!buf->buf)
        sg_sys_abort("invalid record buffer state");

    glBindBuffer(GL_PIXEL_PACK_BUFFER, buf->buf);
    mptr = glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);

    if (mptr) {
        width = buf->width;
        height = buf->height;
        sz = 4 * width * height;
        fptr = malloc(sz);
        if (!fptr) {
            sg_error_nomem(&err);
            sg_logerrs(sg_record.log, SG_LOG_ERROR, err,
                       "could not allocate video buffer");
            sg_error_clear(&err);
        } else {
            for (i = 0; i < height; i++) {
                memcpy(
                    (char *) fptr + (height - i - 1) * (4 * width),
                    (char *) mptr + i * (4 * width),
                    4 * width);
            }
            if (buf->flags & SG_RECORD_FRAME_SCREENSHOT) {
                sg_screenshot_write(fptr, buf->width, buf->height);
            }
            if (buf->flags & SG_RECORD_FRAME_VIDEO) {
                sg_record_writevideo(fptr, buf->width, buf->height);
                fptr = NULL;
            }
            free(fptr);
        }
        glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
    } else {
        sg_logs(sg_record.log, SG_LOG_ERROR,
                "could not map framebuffer for recording");
        sg_record.flags &= ~SG_RECORD_VIDEO;
    }

    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    sg_opengl_checkerror("sg_record_buf_process");

    if ((sg_record.flags & SG_RECORD_VIDEO) == 0) {
        glDeleteBuffers(1, &buf->buf);
        buf->buf = 0;
    }
    buf->flags = 0;
    buf->width = 0;
    buf->height = 0;
}

void
sg_record_init(void)
{
    int i;
    sg_record.log = sg_logger_get("video");
    sg_record.flags = 0;
    for (i = 0; i < 2; i++) {
        sg_record.buf[i].flags = 0;
        sg_record.buf[i].buf = 0;
    }
}

void
sg_record_frame_begin(unsigned *time)
{
    unsigned curtime = *time, nextframe, flags;
    int delta;

    flags = sg_record.flags;
    flags |= SG_RECORD_INFRAME;
    if ((flags & SG_RECORD_VIDEO) != 0) {
        nextframe = sg_videotime_time(&sg_record.vtime);
        delta = curtime - nextframe;
        if (delta >= 0) {
            *time = nextframe;
            flags |= SG_RECORD_FRAME_VIDEO;
            sg_videotime_next(&sg_record.vtime);
        }
    }
    sg_record.flags = flags;
}

void
sg_record_frame_end(int x, int y, int width, int height)
{
    int whichbuf;
    unsigned flags;

    flags = sg_record.flags;
    if ((flags & SG_RECORD_INFRAME) == 0)
        return;
    sg_record.fwidth = width;
    sg_record.fheight = height;
    whichbuf = (flags & SG_RECORD_WHICHBUF) ? 1 : 0;
    sg_record.flags = (flags ^ SG_RECORD_WHICHBUF) &
        ~(SG_RECORD_FRAMEMASK | SG_RECORD_INFRAME);

    if ((flags & SG_RECORD_FRAMEMASK) != 0) {
        sg_record_buf_read(&sg_record.buf[whichbuf],
                           x, y, width, height,
                           flags & SG_RECORD_FRAMEMASK);
    }

    if (sg_record.buf[!whichbuf].flags != 0) {
        sg_record_buf_process(&sg_record.buf[!whichbuf]);
    }

    flags = sg_record.flags;
    if ((flags & SG_RECORD_VIDEO) == 0 &&
        (flags & (SG_RECORD_HASVIO | SG_RECORD_HASVPROC)) != 0) {
        if ((sg_record.flags & SG_RECORD_HASVIO) != 0) {
            flags = sg_record.buf[0].flags | sg_record.buf[1].flags;
            if ((flags & SG_RECORD_FRAME_VIDEO) == 0)
                sg_videoio_stop(&sg_record.vio);
            if (!sg_videoio_poll(&sg_record.vio))
                sg_record_stopvio();
        }
        if ((sg_record.flags & SG_RECORD_HASVPROC) != 0) {
            if (!sg_videoproc_poll(&sg_record.vproc))
                sg_record_stopvproc();
        }
    }
}

void
sg_record_screenshot(void)
{
    sg_record.flags |= SG_RECORD_FRAME_SCREENSHOT;
}

#define MAX_EXTENSION 16

void
sg_record_start(unsigned timestamp)
{
    struct sg_videoparam param;
    char path[SG_DATE_LEN + 4 + MAX_EXTENSION];
    int r, width = sg_record.fwidth, height = sg_record.fheight;
    size_t framebytes, extlen;
    unsigned mask;
    struct sg_error *err = NULL;

    mask = SG_RECORD_VIDEO | SG_RECORD_HASVIO | SG_RECORD_HASVPROC;
    if ((sg_record.flags & mask) != 0)
        return;

    if (width == 0 || height == 0)
        return;
    framebytes = (size_t) 4 * width * height;

    sg_videoparam_get(&param);

    memcpy(path, "./", 2);
    r = sg_clock_getdate(path + 2, 1);
    assert(r >= 0 && r < SG_DATE_LEN);
    r += 2;
    extlen = strlen(param.extension);
    if (extlen > MAX_EXTENSION) {
        sg_logs(sg_record.log, SG_LOG_ERROR,
                "video extension too long");
        return;
    }
    if (param.extension[0] != '.') {
        path[r] = '.';
        r++;
    }
    memcpy(path + r, param.extension, extlen);
    r += extlen;
    path[r] = '\0';

    sg_videotime_init(&sg_record.vtime, timestamp,
                      param.rate_numer, param.rate_denom);

    r = sg_videoproc_init(&sg_record.vproc, &param,
                          path, width, height, &err);
    if (r) {
        sg_logerrs(sg_record.log, SG_LOG_ERROR, err,
                   "could not start video encoder");
        sg_error_clear(&err);
        return;
    }

    r = sg_videoio_init(&sg_record.vio, framebytes,
                        sg_record.vproc.pipe, &err);
    if (r) {
        sg_logerrs(sg_record.log, SG_LOG_ERROR, err,
                   "could not start video encoder IO thread");
        sg_error_clear(&err);
        sg_videoproc_destroy(&sg_record.vproc, 1);
        return;
    }

    sg_record.flags |= mask;
    sg_record.vwidth = width;
    sg_record.vheight = height;
    sg_record.framebytes = framebytes;
}

void
sg_record_stop(void)
{
    sg_record.flags &= ~SG_RECORD_VIDEO;
}
