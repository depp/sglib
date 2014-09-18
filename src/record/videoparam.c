/* Copyright 2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "videoparam.h"
#include "sg/cvar.h"

const char SG_VIDEO_SECTION[] = "video";

void
sg_videoparam_get(struct sg_videoparam *p)
{
    int rate;

    p->rate_numer = 30;
    p->rate_denom = 1000;
    if (sg_cvar_geti(SG_VIDEO_SECTION, "rate", &rate)) {
        if (rate >= 1 && rate <= 60)
            p->rate_numer = rate;
    }

    if (!sg_cvar_gets(SG_VIDEO_SECTION, "command", &p->command))
        p->command = "ffmpeg";

    if (!sg_cvar_gets(SG_VIDEO_SECTION, "arguments", &p->arguments))
        p->arguments = "-codec:v libx264 -preset fast -crf 18";

    if (!sg_cvar_gets(SG_VIDEO_SECTION, "extension", &p->extension))
        p->extension = "mp4";
}
