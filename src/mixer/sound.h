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
    pce_atomic_t refcount;
    const char *path;
    size_t pathlen;
    struct sg_mixer_sample sample;
};
