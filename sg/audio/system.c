/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "sg/audio_system.h"
#include "sg/log.h"
#include "sysprivate.h"

struct sg_audio_system sg_audio_system_global;

void
sg_audio_sys_init(void)
{
    struct sg_audio_system *sp = &sg_audio_system_global;
    sp->log = sg_logger_get("audio");
    pce_lock_init(&sp->slock);
    pce_rwlock_init(&sp->qlock);
    sp->srcfree = -1;
}
