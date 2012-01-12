#include "clock.h"
#include "entry.h"
#include "event.h"
#include "lfile.h"
#include "rand.h"

static int sg_vid_width, sg_vid_height;

void
sg_sys_init(void)
{
    sg_path_init();
    sg_clock_init();
    sg_rand_seed(&sg_rand_global, 1);
    sg_game_init();
}

void
sg_sys_getsize(int *width, int *height)
{
    sg_game_getsize(width, height);
}

void
sg_sys_event(union sg_event *evt)
{
    if (evt->type == SG_EVENT_RESIZE) {
        sg_vid_width = evt->resize.width;
        sg_vid_height = evt->resize.height;
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
