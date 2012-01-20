#include "clock.h"
#include "entry.h"
#include "event.h"
#include "file.h"
#include "log.h"
#include "rand.h"
#include "version.h"
#include <stdio.h>

static unsigned sg_status;
static int sg_vid_width, sg_vid_height;

void
sg_sys_init(void)
{
    struct sg_logger *log;
    sg_log_init();
    log = sg_logger_get("sys");
    sg_logf(log, LOG_INFO, "Version: %s", VERSION_STRING);
    sg_path_init();
    sg_clock_init();
    sg_rand_seed(&sg_rand_global, 1);
    sg_game_init();
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
    switch (evt->type) {
    default:
        break;

    case SG_EVENT_RESIZE:
        sg_vid_width = evt->resize.width;
        sg_vid_height = evt->resize.height;
        break;

    case SG_EVENT_STATUS:
        sg_status = evt->status.status;
        if (sg_status & SG_STATUS_VISIBLE) {
            if (sg_status & SG_STATUS_FULLSCREEN)
                puts("Status: fullscreen");
            else
                puts("Status: windowed");
        } else {
            puts("Status: hidden");
        }
        break;
    }
    sg_game_event(evt);
}

void
sg_sys_draw(void)
{
    unsigned msec = sg_clock_get();
    sg_game_draw(0, 0, sg_vid_width, sg_vid_height, msec);
}

void
sg_sys_destroy(void)
{
    sg_game_destroy();
}
