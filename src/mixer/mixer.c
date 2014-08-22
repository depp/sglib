/* Copyright 2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "mixer.h"
#include "../core/private.h"
#include <stdlib.h>
#include <string.h>

struct sg_mixer sg_mixer;

void
sg_mixer_init(void)
{
    unsigned count;

    sg_mixer_system_init();
    pce_lock_init(&sg_mixer.lock);

    count = 64;
    sg_mixer.channel = calloc(sizeof(*sg_mixer.channel), count);
    if (!sg_mixer.channel)
        abort(); /* FIXME: do something smart */
    sg_mixer.channelcount = count;
}

void
sg_mixer_settime(unsigned timestamp)
{
    sg_mixer.time = timestamp;
}

/* Update all channel flags.  Requires lock.  */
static void
sg_mixer_commitflags(void)
{
    struct sg_mixer_channel *chp, *che;
    unsigned endflags;
    endflags = 0;
    if (sg_mixer.mix_live)
        endflags |= SG_MIXER_GFLAG_DONE1;
    if (sg_mixer.mix_record)
        endflags |= SG_MIXER_GFLAG_DONE2;
    chp = sg_mixer.channel;
    che = chp + sg_mixer.channelcount;
    for (; chp != che; chp++) {
        if (!chp->lflags)
            continue;
        if (chp->lflags & SG_MIXER_LFLAG_START)
            chp->gflags |= SG_MIXER_GFLAG_START;
        if (chp->lflags & SG_MIXER_LFLAG_STOP)
            chp->gflags |= SG_MIXER_GFLAG_STOP;
        if (chp->lflags & SG_MIXER_LFLAG_LOOP)
            chp->gflags |= SG_MIXER_GFLAG_LOOP;
        if ((chp->gflags & endflags) == endflags) {
            chp->lflags |= SG_MIXER_LFLAG_DONE;
            if (chp->lflags & SG_MIXER_LFLAG_DETACHED) {
                chp->lflags = 0;
                chp->gflags = 0;
            }
        }
    }
}

/* Send committed messages to the mixdowns.  Requires lock.  */
static void
sg_mixer_commitmsg(void)
{
    struct sg_mixer_msg *imsg, *omsg;
    unsigned nmsg, i, ch, param;
    imsg = sg_mixer.queue.msg;
    nmsg = sg_mixer.queue.msgcount;
    if (sg_mixer.mix_live) {
        omsg = sg_mixer_queue_append(&sg_mixer.mix_live->inqueue, nmsg);
        if (omsg)
            memcpy(omsg, imsg, sizeof(*omsg) * nmsg);
    }
    if (sg_mixer.mix_record) {
        omsg = sg_mixer_queue_append(&sg_mixer.mix_live->inqueue, nmsg);
        if (omsg)
            memcpy(omsg, imsg, sizeof(*omsg) * nmsg);
    }
    for (i = 0; i < nmsg; i++) {
        ch = imsg[i].addr >> 16;
        param = imsg[i].addr & 0xffff;
        sg_mixer.channel[ch].param_cur[param] = imsg[i].value;
    }
    sg_mixer.queue.msgcount = 0;
}

/* Clean up channels which have completed.  */
static void
sg_mixer_cleanup(void)
{
    struct sg_mixer_channel *chp, *che;
    unsigned doneflags = SG_MIXER_LFLAG_STOP | SG_MIXER_LFLAG_DONE;
    chp = sg_mixer.channel;
    che = chp + sg_mixer.channelcount;
    for (; chp != che; chp++) {
        if ((chp->lflags & doneflags) != doneflags)
            continue;
        chp->lflags = 0;
        if (chp->sound) {
            sg_mixer_sound_decref(chp->sound);
            chp->sound = NULL;
        }
    }
}

void
sg_mixer_commit(void)
{
    pce_lock_acquire(&sg_mixer.lock);
    sg_mixer_commitflags();
    sg_mixer_commitmsg();
    sg_mixer.is_ready = 1;
    sg_mixer.committime = sg_mixer.time;
    pce_lock_release(&sg_mixer.lock);

    sg_mixer_cleanup();
}
