/* Copyright 2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */

/* Initialize screenshot and video recording subsystem */
void
sg_record_init(void);

/* Start recording a frame.  This will adjust the timestamp of the
   frame to be rendered.  */
void
sg_record_frame_begin(double *time);
