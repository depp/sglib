/* Copyright 2013-2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "defs.h"
#include "sg/error.h"
#include "sg/log.h"
#include "sg/mixer.h"
#include "sg/log.h"
#include <string.h>

struct sg_mixer_sound *
load_audio(const char *path)
{
    struct sg_mixer_sound *snd;
    struct sg_error *err = NULL;

    snd = sg_mixer_sound_file(path, strlen(path), &err);
    if (!snd) {
        sg_logf(sg_logger_get(NULL), SG_LOG_ERROR,
                "could not load audio file: %s", path);
        sg_error_clear(&err);
    }

    return snd;
}
