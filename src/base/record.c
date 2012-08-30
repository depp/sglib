#include "dispatch.h"
#include "defs.h"
#include "entry.h"
#include "opengl.h"
#include "record.h"
#include <stdlib.h>

enum {
    SG_REC_SHOT = 1,
    SG_REC_VID = 2
};

struct sg_rec_buf {
    GLuint buf;
    int width, height;
    int reqflags;
};

struct sg_rec_state {
    /* Used for scheduling sg_record_callback */
    int excl;
    /* which: the index of the buffer which receives the next
       screenshot.  The other buffer has the previous screenshot.  */
    int which;
    /* If 0, any width or height is fine.  If nonzero, then use these
       dimensions to OVERRIDE the dimensions of the main display, for
       the purposes of recording video.  You can't change video
       dimensions in the middle of recording.  */
    int width, height;
    struct sg_rec_buf buf[2];
};

static struct sg_rec_state sg_rec_state;

static void
sg_record_schedule(void);

/* Record pixels into the given buffer.  */
static void
sg_record_readpix(struct sg_rec_buf *SG_RESTRICT buf)
{
    int width, height;

    if (!buf->buf) {
        glGenBuffers(1, &buf->buf);
        if (!buf->buf)
            goto err;
    }

    if (sg_rec_state.width) {
        width = sg_rec_state.width;
        height = sg_rec_state.height;
    } else {
        width = sg_sst.width;
        height = sg_sst.height;
    }

    if (width > 0x8000 || height > 0x8000)
        goto err;

    glBindBuffer(GL_PIXEL_PACK_BUFFER, buf->buf);
    glBufferData(GL_PIXEL_PACK_BUFFER, width * height * 4,
                 NULL, GL_STREAM_READ);
    glReadBuffer(GL_FRONT);
    glReadPixels(0, 0, width, height,
                 GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

    sg_record_schedule();

    buf->width = width;
    buf->height = height;

    return;

err:
    abort();
}

/* Process pixels in the given buffer.  */
static void
sg_record_procpix(struct sg_rec_buf *SG_RESTRICT buf)
{
    void *mptr;

    if (!buf->buf)
        goto err;

    glBindBuffer(GL_PIXEL_PACK_BUFFER, buf->buf);
    mptr = glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);

    if (!mptr)
        goto err;

    if (buf->reqflags & SG_REC_SHOT)
        sg_record_writeshot(mptr, buf->width, buf->height);

    if (buf->reqflags & SG_REC_VID)
        sg_record_writevideo(mptr, buf->width, buf->height);

    glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

    if (!sg_rec_state.width) {
        glDeleteBuffers(1, &buf->buf);
        buf->buf = 0;
    }
    buf->width = 0;
    buf->height = 0;
    buf->reqflags = 0;

    return;

err:
    abort();
}

/* End of frame callback for processing screenshots and video.  */
static void
sg_record_callback(void *cxt)
{
    int which = sg_rec_state.which;

    (void) cxt;

    if (sg_rec_state.buf[which].reqflags) {
        sg_record_readpix(&sg_rec_state.buf[which]);
        sg_rec_state.which = !which;
    }

    if (sg_rec_state.buf[!which].reqflags) {
        sg_record_procpix(&sg_rec_state.buf[!which]);
    }
}

static void
sg_record_schedule(void)
{
    sg_dispatch_sync_queue(SG_POST_RENDER, 0, &sg_rec_state.excl,
                           NULL, sg_record_callback);
}

void
sg_record_screenshot(void)
{
    sg_rec_state.buf[sg_rec_state.which].reqflags |= SG_REC_SHOT;
    sg_record_schedule();
}

void
sg_record_vidframe(void)
{
    sg_rec_state.buf[sg_rec_state.which].reqflags |= SG_REC_VID;
    sg_record_schedule();
}

void
sg_record_fixsize(int width, int height)
{
    sg_rec_state.width = width;
    sg_rec_state.height = height;
}
