/* Copyright 2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */

/* Write a screenshot to disk.  The buffer must contain RGBX data.  */
void
sg_screenshot_write(const void *ptr, int width, int height);
