#include "clock.h"
#include "dispatch.h"
#include "entry.h"
#include "event.h"
#include "file.h"
#include "log.h"
#include "rand.h"
#include "version.h"

static unsigned sg_status;
static int sg_vid_width, sg_vid_height;
static struct sg_logger *sg_log_video;

void
sg_sys_init(void)
{
    struct sg_logger *log;
    sg_log_init();
    log = sg_logger_get("init");
    if (LOG_INFO >= log->level)
        sg_version_print();
    sg_path_init();
    sg_dispatch_async_init();
    sg_dispatch_sync_init();
    sg_clock_init();
    sg_rand_seed(&sg_rand_global, 1);
    sg_game_init();
    sg_log_video = sg_logger_get("video");
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
        sg_vid_width = evt->resize.width;
        sg_vid_height = evt->resize.height;
        break;

    case SG_EVENT_STATUS:
        sg_status = evt->status.status;
        if (LOG_INFO >= sg_log_video->level) {
            if (sg_status & SG_STATUS_VISIBLE) {
                if (sg_status & SG_STATUS_FULLSCREEN)
                    status = "fullscreen";
                else
                    status = "windowed";
            } else {
                status = "hidden";
            }
            sg_logf(sg_log_video, LOG_INFO, "Status: %s", status);
        }
        break;
    }
    sg_game_event(evt);
}

void
sg_sys_draw(void)
{
    unsigned msec;
    sg_dispatch_sync_run();
    msec = sg_clock_get();
    sg_game_draw(0, 0, sg_vid_width, sg_vid_height, msec);
}

void
sg_sys_destroy(void)
{
    sg_game_destroy();
}
