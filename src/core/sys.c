/* Copyright 2012-2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "sg/entry.h"
#include "sg/log.h"
#include "sg/rand.h"
#include "sg/version.h"
#include "private.h"
#include <stdio.h>

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

void
sg_sys_init(void)
{
    struct sg_logger *log;
    sg_log_init();
    log = sg_logger_get("init");
    if (SG_LOG_INFO >= log->level)
        sg_version_print();
    sg_path_init();
    sg_clock_init();
    sg_rand_seed(&sg_rand_global, 1);
    sg_mixer_init();
    sg_game_init();
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
