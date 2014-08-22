/* Copyright 2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "sg/atomic.h"
#include <stddef.h>

struct sg_mixer_sample {
    short *data;
    int stereo;
    unsigned length;
};

struct sg_mixer_sound {
    sg_atomic_t refcount;
    sg_atomic_t is_loaded;
    const char *path;
    int pathlen;
    struct sg_mixer_sample sample;
};

/* Initialize the sound subsystem.  */
void
sg_mixer_sound_init(void);

/* Set the sample rate for all sounds, or set to zero to unload all
   sounds.  Sounds will not be loaded into memory until the sample
   rate is set.  */
void
sg_mixer_sound_setrate(int rate);
