/* Copyright 2012-2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "sg/entry.h"
#include "sg/cvar.h"
#include "sg/log.h"
#include "sg/rand.h"
#include "sg/record.h"
#include "sg/version.h"
#include "../record/record.h"
#include "private.h"
#include <stdio.h>
#include <string.h>

struct sg_sys sg_sys;

#if defined __unix__ || defined __APPLE__ && defined __MACH__
#include <signal.h>

static void
sg_sys_siginit(void)
{
    signal(SIGPIPE, SIG_IGN);
}

#else

#define sg_sys_siginit() (void)0

#endif

const struct sg_game_info sg_game_info_defaults = {
    /* minimum width, height */
    1280 / 4, 720 / 4,

    /* default width, height */
    1280, 720,

    /* min, max aspect ratio */
    1.25, 4.0,

    /* name */
    "Game"
};

static void
sg_sys_parseargs(
    int argc,
    char **argv)
{
    int i;
    const char *arg, *p;
    for (i = 0; i < argc; i++) {
        arg = argv[i];
        if (!((*arg >= 'a' && *arg <= 'z') ||
              (*arg >= 'A' && *arg <= 'Z') ||
              *arg == '_'))
            continue;
        p = strchr(arg, '=');
        if (!p)
            continue;
        sg_cvar_set(arg, p - arg, p + 1, SG_CVAR_CREATE);
    }
}

void
sg_sys_init(
    int argc,
    char **argv)
{
    sg_sys_siginit();

    /* Take the most direct path to loading the config file, so other
       systems will have their configuration available.  Loading the
       config file requires the file subsystem, the file subsystem
       needs command-line arguments, and pretty much everything
       requires logging, which requires a working clock.  */
    sg_clock_init();
    sg_log_init();
    sg_sys_parseargs(argc, argv);
    sg_path_init();
    sg_cvar_loadfile("config", strlen("config"),
                     SG_CVAR_CREATE | SG_CVAR_PERSISTENT, NULL);

    sg_version_print();
    sg_rand_seed(&sg_rand_global, 1);
    sg_mixer_init();
    sg_record_init();
    sg_game_init();

    sg_cvar_defbool("video", "showfps", "Enable frame rate logging",
                    &sg_sys.showfps, 0, SG_CVAR_PERSISTENT);
    sg_cvar_defint("video", "vsync", "Vertical sync mode (0, 1, or 2)",
                   &sg_sys.vsync, 0, 0, 2, SG_CVAR_PERSISTENT);
    sg_cvar_defint("video", "maxfps", "Frame rate cap (0 to disable)",
                   &sg_sys.maxfps, 120, 0, 1000, SG_CVAR_PERSISTENT);

    sg_cvar_save("config.ini", strlen("config.ini"), 0, NULL);
}

void
sg_sys_getinfo(struct sg_game_info *info)
{
    info->min_width = 320;
    info->min_height = 180;
    info->default_width = 1280;
    info->default_height = 720;
    info->min_aspect = 1.25;
    info->max_aspect = 2.0;
    sg_game_getinfo(info);
    if (info->min_width < 1)
        info->min_width = 1;
    if (info->min_height < 1)
        info->min_height = 1;
    if (info->default_width < info->min_width)
        info->default_width = info->min_width;
    if (info->default_height < info->min_height)
        info->default_height = info->min_height;
    if (info->min_aspect < 0.125)
        info->min_aspect = 0.125;
    if (info->max_aspect > 8.0)
        info->max_aspect = 8.0;
}

void
sg_sys_draw(int width, int height, double time)
{
    static int time_ref, frame_count;
    int time_delta;

    frame_count++;
    time_delta = time - time_ref;
    if (time_delta > 1000) {
        if (sg_sys.showfps.value) {
            sg_logf(SG_LOG_INFO, "FPS: %.0f",
                    1000.0 * frame_count / time_delta);
        }
        time_ref = time;
        frame_count = 0;
    }
    double adjtime = time;
    sg_record_frame_begin(&adjtime);
    sg_game_draw(width, height, adjtime);
    sg_record_frame_end(0, 0, width, height);
}

void
sg_sys_abortf(const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    sg_sys_abortv(msg, ap);
    va_end(ap);
}

void
sg_sys_abortv(const char *msg, va_list ap)
{
    char buf[512];
    buf[0] = '\0';
#if defined(_MSC_VER)
    vsnprintf_s(buf, sizeof(buf), _TRUNCATE, msg, ap);
#else
    vsnprintf(buf, sizeof(buf), msg, ap);
#endif
    sg_sys_abort(buf);
}
