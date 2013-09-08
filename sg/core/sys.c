/* Copyright 2012-2013 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "config.h"
#include "keycode/keycode.h"
#include "sg/audio_pcm.h"
#include "sg/audio_system.h"
#include "sg/aio.h"
#include "sg/clock.h"
#include "sg/dispatch.h"
#include "sg/entry.h"
#include "sg/event.h"
#include "sg/file.h"
#include "sg/log.h"
#include "sg/rand.h"
#include "sg/record.h"
#include "sg/version.h"
#include "../private.h"

struct sg_sys_state sg_sst;
static struct sg_logger *sg_log_video;

#if 0
#include <stdio.h>
static void
callback(void *cxt)
{
    int i = (int) (long) cxt;
    printf("callback: %s %d\n",
           (i & 1) ? "post" : "pre ",
           (i >> 1) ^ 0x33);
}

static void
test_callback(void)
{
    int i;
    for (i = 0; i < 128; ++i)
        sg_dispatch_sync_queue(
            SG_PRE_RENDER + (i & 1), (i >> 1) ^ 0x33, NULL,
            (void *) (long) i, callback);
    for (i = 0; i < 16; ++i)
        sg_dispatch_sync_queue(
            SG_PRE_RENDER, 32, NULL,
            (void *) (long) ((32 ^ 0x33) << 1), callback);
}
#else
#define test_callback()
#endif

void
sg_sys_init(void)
{
    struct sg_logger *log;
    sg_log_init();
    log = sg_logger_get("init");
    if (LOG_INFO >= log->level)
        sg_version_print();
    sg_path_init();
    sg_dispatch_init();
    sg_aio_init();
    sg_clock_init();
    sg_rand_seed(&sg_rand_global, 1);
    sg_audio_sys_init();
    sg_game_init();
    sg_log_video = sg_logger_get("video");

    test_callback();
}

void
sg_sys_getinfo(struct sg_game_info *info)
{
    info->min_width = 320;
    info->min_height = 180;
    info->default_width = 1280;
    info->default_height = 720;
    info->min_aspect = SG_GAME_ASPECT_SCALE * 5 / 4;
    info->max_aspect = SG_GAME_ASPECT_SCALE * 2;
    sg_game_getinfo(info);
}

void
sg_sys_event(union sg_event *evt)
{
    const char *status;
    switch (evt->type) {
    default:
        break;

    case SG_EVENT_RESIZE:
        sg_sst.width = evt->resize.width;
        sg_sst.height = evt->resize.height;
        break;

    case SG_EVENT_STATUS:
        sg_sst.status = evt->status.status;
        if (LOG_INFO >= sg_log_video->level) {
            if (sg_sst.status & SG_STATUS_VISIBLE) {
                if (sg_sst.status & SG_STATUS_FULLSCREEN)
                    status = "fullscreen";
                else
                    status = "windowed";
            } else {
                status = "hidden";
            }
            sg_logf(sg_log_video, LOG_INFO, "Status: %s", status);
        }
        break;

    case SG_EVENT_KDOWN:
        switch (evt->key.key) {
        default:
            break;

#if defined(ENABLE_VIDEO_RECORDING)
        case KEY_F10:
            if (sg_sst.rec_numer) {
                sg_record_vidstop();
            } else {
                sg_record_vidstart();
            }
            break;
#endif

        case KEY_F12:
            sg_record_screenshot();
            break;
        }
        break;
    }
    sg_game_event(evt);
}

void
sg_sys_draw(void)
{
    unsigned msec, n;
    (void) n;

    msec = sg_clock_get() - sg_sst.tick_offset;
#if defined(ENABLE_VIDEO_RECORDING)
    if (sg_sst.rec_numer && (int) (msec - sg_sst.rec_next) >= 0) {
        if ((int) (msec - sg_sst.rec_next) >= 500) {
            sg_logf(sg_log_video, LOG_WARN, "lag over 500ms, adjusting");
            sg_sst.tick_offset += msec - sg_sst.rec_next;
        }
        msec = sg_sst.rec_next;
        n = ++sg_sst.rec_ct;
        if (n == sg_sst.rec_numer) {
            n = sg_sst.rec_ct = 0;
            sg_sst.rec_ref += sg_sst.rec_denom;
        }
        sg_sst.rec_next = sg_sst.rec_ref +
            (2 * n + 1) * sg_sst.rec_denom / (2 * sg_sst.rec_numer);
        sg_record_vidframe();
    }
    sg_sst.tick = msec;
#endif

    sg_game_draw(0, 0, sg_sst.width, sg_sst.height, msec);
}

void
sg_sys_destroy(void)
{
    sg_game_destroy();
}
