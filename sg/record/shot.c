/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "sg/clock.h"
#include "sg/dispatch.h"
#include "sg/error.h"
#include "sg/file.h"
#include "sg/log.h"
#include "sg/pixbuf.h"
#include "sg/record.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static int sg_sshot_counter;

struct sg_sshot {
    unsigned width, height;
    int counter;
    void *ptr;
    char stamp[SG_DATE_LEN];
};

static void
sg_record_shot_writepng(void *cxt)
{
    struct sg_sshot *sp = cxt;
    char name[32];
    struct sg_file *fp;
    struct sg_error *err = NULL;
    struct sg_pixbuf pbuf;
    int r;

    sprintf(name, "shot_%02d.png", sp->counter);

    pbuf.data = sp->ptr;
    pbuf.format = SG_RGBX;
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

void
sg_record_writeshot(unsigned timestamp,
                    const void *ptr, int width, int height)
{
    struct sg_sshot *shot;
    unsigned rb;
    void *iptr;
    int y;

    (void) timestamp;

    shot = malloc(sizeof(*shot));
    if (!shot)
        return;
    rb = width * 4;
    iptr = malloc(height * rb);
    if (!iptr) {
        free(shot);
        return;
    }
    for (y = 0; y < height; ++y) {
        memcpy((unsigned char *) iptr + (height - 1 - y) * rb,
               (unsigned char *) ptr + y * rb, rb);
    }
    shot->width = width;
    shot->height = height;
    shot->counter = ++sg_sshot_counter;
    shot->ptr = iptr;
    sg_clock_getdate(shot->stamp);
    sg_dispatch_queue(0, shot, sg_record_shot_writepng);
}
