/* Copyright 2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "config.h"
#include "sg/cvar.h"

#if defined ENABLE_VIDEO_RECORDING
struct sg_recordcvar {
    struct sg_cvar_bool enable;
    struct sg_cvar_int rate;
    struct sg_cvar_string command;
    struct sg_cvar_string arguments;
    struct sg_cvar_string extension;
};

extern struct sg_recordcvar sg_recordcvar;
#endif

/* Initialize screenshot and video recording subsystem */
void
sg_record_init(void);

/* Start recording a frame.  This will adjust the timestamp of the
   frame to be rendered.  */
void
sg_record_frame_begin(double *time);
