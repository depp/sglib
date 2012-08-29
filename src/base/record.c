#include "clock.h"
#include "dispatch.h"
#include "error.h"
#include "file.h"
#include "log.h"
#include "opengl.h"
#include "pixbuf.h"
#include "record.h"
#include "entry.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct sg_sshot {
    unsigned width, height;
    int counter;
    void *ptr;
    char stamp[SG_DATE_LEN];
};

static void
sg_record_writepng(void *cxt)
{
    struct sg_sshot *sp = cxt;
    char name[32];
    struct sg_file *fp;
    struct sg_error *err = NULL;
    struct sg_pixbuf pbuf;
    int r;

    sprintf(name, "shot_%02d.png", sp->counter);

    pbuf.data = sp->ptr;
    pbuf.format = SG_RGBA;
    pbuf.iwidth = sp->width;
    pbuf.iheight = sp->height;
    pbuf.pwidth = sp->width;
    pbuf.pheight = sp->height;
    pbuf.rowbytes = sp->width * 4;

    fp = sg_file_open(name, strlen(name),
                      SG_WRONLY, NULL, &err);
    if (!fp) goto cleanup;

    r = sg_pixbuf_writepng(&pbuf, fp, &err);
    if (r) goto cleanup;

cleanup:
    if (err) {
        sg_logerrs(sg_logger_get(NULL), LOG_ERROR, err,
                   "could not save screenshot");
        sg_error_clear(&err);
    }
    if (fp)
        fp->close(fp);
    free(sp);
    free(sp->ptr);
}

static void
sg_record_callback(void *cxt);

static void
sg_record_schedule(void)
{
    static int excl;
    sg_dispatch_sync_queue(SG_POST_RENDER, 0, &excl,
                           NULL, sg_record_callback);
}

static void
sg_record_callback(void *cxt)
{
    static int state, counter;
    static GLuint buffer;
    static struct sg_sshot *shot;
    unsigned width, height;
    void *ptr;

    (void) cxt;

    if (!state) {
        if (!buffer)
            glGenBuffers(1, &buffer);
        if (!buffer)
            return;

        width = sg_vid_width;
        height = sg_vid_height;
        if (width > 0x8000 || height > 0x8000)
            return;

        shot = malloc(sizeof(*shot));
        if (!shot)
            return;

        shot->width = width;
        shot->height = height;
        shot->counter = counter++;
        shot->ptr = NULL;
        sg_clock_getdate(shot->stamp);

        glBindBuffer(GL_PIXEL_PACK_BUFFER, buffer);
        glBufferData(GL_PIXEL_PACK_BUFFER, width * height * 4,
                     NULL, GL_STREAM_READ);
        glReadBuffer(GL_FRONT);
        glReadPixels(0, 0, width, height,
                     GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

        state = 1;
        sg_record_schedule();
    } else {
        width = shot->width;
        height = shot->height;

        shot->ptr = malloc(width * height * 4);
        if (!shot->ptr) {
            free(shot);
            goto done;
        }

        glBindBuffer(GL_PIXEL_PACK_BUFFER, buffer);
        ptr = glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);

        if (ptr)
            memcpy(shot->ptr, ptr, width * height * 4);

        glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

        if (ptr) {
            sg_dispatch_async_queue(
                SG_DISPATCH_IO, 0,
                shot, sg_record_writepng);
            shot = NULL;
        }

    done:
        glDeleteBuffers(1, &buffer);
        buffer = 0;
        state = 0;
        if (shot) {
            free(shot->ptr);
            free(shot);
            shot = NULL;
        }
    }
}

void
sg_record_screenshot(void)
{
    sg_record_schedule();
}
