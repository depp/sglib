/* Copyright 2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "mixer.h"

struct sg_mixer_channel *
sg_mixer_channel_play(struct sg_mixer_sound *sound, unsigned timestamp)
{
    struct sg_mixer_channel *chp, *che;
    int i;

    chp = sg_mixer.channel;
    che = chp + sg_mixer.channelcount;
    for (; chp != che; chp++)
        if (chp->lflags == 0)
            break;
    if (chp == che)
        return NULL;

    chp->lflags = SG_MIXER_LFLAG_START | SG_MIXER_LFLAG_INIT;
    chp->starttime = timestamp;
    chp->sound = sound;
    sg_mixer_sound_incref(sound);
    for (i = 0; i < SG_MIXER_PARAM_COUNT; i++) {
        chp->param_init[i] = 0.0f;
        chp->param_cur[i] = 0.0f;
    }
    return chp;
}

void
sg_mixer_channel_stop(struct sg_mixer_channel *channel)
{
    if (!channel)
        return;
    /* FIXME: Logging: don't stop a stopped channel.  */
    channel->stoptime = sg_mixer.time;
    channel->lflags |= SG_MIXER_LFLAG_STOP;
}

int
sg_mixer_channel_isdone(struct sg_mixer_channel *channel)
{
    return !channel || (channel->lflags & SG_MIXER_LFLAG_DONE) != 0;
}

void
sg_mixer_channel_setparam(struct sg_mixer_channel *channel,
                          sg_mixer_param_t param, float value)
{
    struct sg_mixer_param p;
    p.param = param;
    p.value = value;
    sg_mixer_channel_setparams(channel, &p, 1);
}

void
sg_mixer_channel_setparams(struct sg_mixer_channel *channel,
                           struct sg_mixer_param *param, size_t count)
{
    unsigned baseaddr, timestamp;
    size_t i;
    struct sg_mixer_msg *msg;

    if (!count || !channel)
        return;
    if (channel->lflags & SG_MIXER_LFLAG_INIT) {
        for (i = 0; i < count; i++) {
            channel->param_init[param[i].param] = param[i].value;
            channel->param_cur[param[i].param] = param[i].value;
        }
    } else {
        msg = sg_mixer_queue_append(&sg_mixer.queue, count);
        if (!msg)
            return;
        baseaddr = (unsigned) (channel - sg_mixer.channel) << 16;
        timestamp = sg_mixer.time;
        for (i = 0; i < count; i++) {
            msg[i].addr = baseaddr | param[i].param;
            msg[i].timestamp = timestamp;
            msg[i].value = param[i].value;
        }
    }
}
