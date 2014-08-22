/* Copyright 2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "mixer.h"
#include "sg/util.h"
#include <stdlib.h>

#pragma GCC diagnostic ignored "-Wtype-limits"

void
sg_mixer_queue_init(struct sg_mixer_queue *queue)
{
    queue->msg = NULL;
    queue->msgcount = 0;
    queue->msgalloc = 0;
}

void
sg_mixer_queue_destroy(struct sg_mixer_queue *queue)
{
    free(queue->msg);
}

struct sg_mixer_msg *
sg_mixer_queue_append(struct sg_mixer_queue *SG_RESTRICT queue,
                      unsigned count)
{
    unsigned nalloc;
    struct sg_mixer_msg *msg;

    if (queue->msgalloc - queue->msgcount >= count) {
        msg = queue->msg + queue->msgcount;
        queue->msgcount += count;
        return msg;
    }

    if (count > (unsigned) -1 - queue->msgcount)
        goto nomem;
    nalloc = sg_round_up_pow2_32(queue->msgcount + count);
    if (!nalloc || nalloc > (size_t) -1 / sizeof(struct sg_mixer_msg))
        goto nomem;
    msg = realloc(queue->msg, sizeof(struct sg_mixer_msg) * nalloc);
    if (!msg)
        goto nomem;
    queue->msg = msg;
    queue->msgalloc = nalloc;
    msg += queue->msgcount;
    queue->msgcount += count;
    return msg;

nomem:
    return NULL;
}
