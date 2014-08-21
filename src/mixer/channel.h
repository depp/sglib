/* Copyright 2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "sg/mixer.h"

enum {
    SG_MIXER_PARAM_POINTS = 4
};

struct sg_mixer_channel {
    float param[SG_MIXER_PARAM_POINTS][SG_MIXER_PARAM_COUNT];
};

struct sg_mixer {
    struct sg_mixer_channel *channel;
    unsigned channelcount;
    unsigned parampos;
};

extern struct sg_mixer sg_mixer;
