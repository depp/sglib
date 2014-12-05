/* Copyright 2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "private.h"
#include "sg/clock.h"
#include "sg/entry.h"
#include "sg/log.h"

#include <stdlib.h>
#include <string.h>

struct sg_timers {
    struct sg_timer *timer;
    unsigned timercount;
    unsigned timeralloc;
};

struct sg_timer {
    double time;
    sg_timer_func_t callback;
    void *cxt;
};

static struct sg_timers sg_timers;

static void
sg_timer_abort(void)
    SG_ATTR_NORETURN;

static void
sg_timer_abort(void)
{
    sg_sys_abort("Timer error.");
}

void
sg_timer_invoke(void)
{
    struct sg_timer timer;
    double time = sg_clock_get();
    while (sg_timers.timercount > 0 && sg_timers.timer[0].time <= time) {
        timer = sg_timers.timer[0];
        sg_timers.timercount--;
        memmove(sg_timers.timer, sg_timers.timer + 1,
                sizeof(struct sg_timer) * sg_timers.timercount);
        timer.callback(sg_clock_get(), timer.cxt);
    }
}

void
sg_timer_register(
    double time,
    unsigned flags,
    sg_timer_func_t callback,
    void *cxt)
{
    struct sg_timer *p = sg_timers.timer, *e = p + sg_timers.timercount;

    switch (flags & (SG_TIMER_ABSTIME | SG_TIMER_RELTIME)) {
    case SG_TIMER_ABSTIME: break;
    case SG_TIMER_RELTIME: time += sg_clock_get() + time; break;
    default:
        sg_logs(SG_LOG_ERROR, "Invalid timer flags.");
        return;
    }

    if ((flags & (SG_TIMER_KEEP_FIRST | SG_TIMER_KEEP_LAST)) != 0) {
        p = sg_timers.timer;
        e = p + sg_timers.timercount;
        for (; p != e; p++) {
            if (p->callback != callback || p->cxt != cxt)
                continue;
            switch (flags & (SG_TIMER_KEEP_FIRST | SG_TIMER_KEEP_LAST)) {
            case SG_TIMER_KEEP_FIRST:
                if (p->time <= time)
                    return;
                break;
            case SG_TIMER_KEEP_LAST:
                if (p->time >= time)
                    return;
                break;
            default:
                return;
            }
            memmove(p, p + 1, sizeof(*p) * (e - (p + 1)));
            sg_timers.timercount -= 1;
            break;
        }
    }

    if (sg_timers.timercount >= sg_timers.timeralloc) {
        unsigned nalloc;
        struct sg_timer *narr;
        nalloc = sg_timers.timeralloc ? sg_timers.timeralloc * 2 : 1;
        if (!nalloc) {
            sg_timer_abort();
            return;
        }
        narr = realloc(sg_timers.timer, nalloc * sizeof(*narr));
        if (!narr) {
            sg_timer_abort();
            return;
        }
        sg_timers.timer = narr;
        sg_timers.timeralloc = nalloc;
    }

    p = sg_timers.timer;
    e = p + sg_timers.timercount++;
    while (p != e && p->time <= time)
        p++;
    if (p != e) {
        memmove(p + 1, p, sizeof(*p) * (e - p));
    }
    p->time = time;
    p->callback = callback;
    p->cxt = cxt;
}
