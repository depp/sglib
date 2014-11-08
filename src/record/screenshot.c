/* Copyright 2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "screenshot.h"
#include "sg/clock.h"
#include "sg/error.h"
#include "sg/log.h"
#include "sg/pixbuf.h"
#include <assert.h>
#include <string.h>

void
sg_screenshot_write(const void *ptr, int width, int height)
{
    char name[SG_DATE_LEN + 4];
    struct sg_error *err = NULL;
    struct sg_pixbuf pbuf;
    int r;

    pbuf.data = (void *) ptr;
    pbuf.format = SG_RGBX;
    pbuf.width = width;
    pbuf.height = height;
    pbuf.rowbytes = width * 4;

    r = sg_clock_getdate(name, 1);
    assert(r >= 0 && r < SG_DATE_LEN);
    memcpy(name + r, ".png", 5);
    r = sg_pixbuf_writepng(&pbuf, name, r + 4, &err);
    if (r) {
        sg_logerrs(sg_logger_get(NULL), SG_LOG_ERROR, err,
                   "could not save screenshot");
        sg_error_clear(&err);
    }
}
