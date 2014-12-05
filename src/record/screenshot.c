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
    char name[SG_DATE_LEN + 16]; /* screenshot/.png\0 */
    struct sg_error *err = NULL;
    struct sg_pixbuf pbuf;
    int len1, len2, r;

    pbuf.data = (void *) ptr;
    pbuf.format = SG_RGBX;
    pbuf.width = width;
    pbuf.height = height;
    pbuf.rowbytes = width * 4;

    len1 = strlen("screenshot/");
    memcpy(name, "screenshot/", len1);
    len2 = sg_clock_getdate(name + len1, 1);
    assert(len2 >= 0 && len2 < SG_DATE_LEN);
    memcpy(name + len1 + len2, ".png", 5);
    r = sg_pixbuf_writepng(&pbuf, name, len1 + len2 + 4, &err);
    if (r) {
        sg_logerrs(SG_LOG_ERROR, err, "Could not save screenshot.");
        sg_error_clear(&err);
    } else {
        sg_logf(SG_LOG_INFO, "Saved screenshot: %s", name);
    }
}
