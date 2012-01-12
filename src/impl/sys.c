#include "clock.h"
#include "entry.h"
#include "lfile.h"
#include "rand.h"

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
    sg_game_event(evt);
}

void
sg_sys_draw(void)
{
    unsigned msecs = sg_clock_get();
    sg_game_draw(msecs);
}

void
sg_sys_destroy(void)
{
    sg_game_destroy();
}
