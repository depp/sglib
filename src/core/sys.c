/* Copyright 2012-2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "private.h"
#include "sg/entry.h"
#include "sg/cvar.h"
#include "sg/log.h"
#include "sg/rand.h"
#include "sg/record.h"
#include "sg/version.h"
#include "../record/record.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct sg_sys sg_sys;

#define SG_SYS_SIZESZ 16
static char sg_sys_defaultsize[SG_SYS_SIZESZ];

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

static const struct sg_game_info SG_GAME_INFO_DEFAULTS = {
    /* minimum width, height */
    320, 180,

    /* default width, height */
    1280, 720,

    /* min, max aspect ratio */
    1.25, 4.0,

    /* flags */
    SG_GAME_ALLOW_HIDPI |
    SG_GAME_ALLOW_RESIZE,

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

struct sg_sys_size {
    short width, height;
    char name[6];
};

static const struct sg_sys_size SG_SYS_SIZES[] = {
    { 640, 360, "360p" },
    { 1280, 720, "720p" },
    { 1920, 1080, "1080p" },
    { 2560, 1440, "1440p" }
};

/* Buffer must have size SG_SYS_SIZESZ.  */
static void
sg_sys_fmtsize(
    char *buf,
    int width,
    int height)
{
    const struct sg_sys_size *sp = SG_SYS_SIZES,
        *se = sp + sizeof(SG_SYS_SIZES) / sizeof(*SG_SYS_SIZES);
    for (; sp != se; sp++) {
        if (width == sp->width && height == sp->height) {
            strcpy(buf, sp->name);
            return;
        }
    }
#if defined _WIN32
    _snprinf_s(buf, SG_SYS_SIZESZ, _TRUNCATE, "%dx%d", width, height);
#else
    snprintf(buf, SG_SYS_SIZESZ, "%dx%d", width, height);
#endif
}

int
sg_sys_getvidsize(
    int *width,
    int *height)
{
    const struct sg_sys_size *sp = SG_SYS_SIZES,
        *se = sp + sizeof(SG_SYS_SIZES) / sizeof(*SG_SYS_SIZES);
    const char *val = sg_sys.vidsize.value, *p;
    char *end;
    unsigned long lw, lh;

    for (; sp != se; sp++) {
        if (!strcmp(sp->name, val)) {
            *width = sp->width;
            *height = sp->height;
            return 0;
        }
    }

    lw = strtoul(val, &end, 10);
    if (end == val)
        goto invalid;
    p = end;
    while (*p == ' ' || *p == '\t')
        p++;
    if (*p != 'x')
        goto invalid;
    p++;
    lh = strtoul(p, &end, 10);
    if (end == p)
        goto invalid;
    p = end;
    while (*p == ' ' || *p == '\t')
        p++;
    if (*p)
        goto invalid;

    if (lw < 1 || lw > 0x8000 || lh < 1 || lh > 0x8000) {
        sg_logf(SG_LOG_WARN, "Video size too extreme: %lux%lu.", lw, lh);
        return -1;
    }
    *width = (int) lw;
    *height = (int) lh;
    return 0;

invalid:
    sg_logf(SG_LOG_WARN, "Invalid video size: \"%s\".", val);
    return -1;
}

void
sg_sys_init(
    struct sg_game_info *info,
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
    sg_cvar_loadcfg();

    sg_version_print();
    sg_rand_seed(&sg_rand_global, 1);
    sg_mixer_init();
    sg_record_init();
    sg_game_init();

    memcpy(info, &SG_GAME_INFO_DEFAULTS, sizeof(*info));
    sg_game_getinfo(info);

    sg_cvar_defbool("video", "showfps", "Enable frame rate logging",
                    &sg_sys.showfps, 0, SG_CVAR_PERSISTENT);
    sg_cvar_defint("video", "vsync", "Vertical sync mode (0, 1, or 2)",
                   &sg_sys.vsync, 0, 0, 2, SG_CVAR_PERSISTENT);
    sg_cvar_defint("video", "maxfps", "Frame rate cap (0 to disable)",
                   &sg_sys.maxfps, 150, 0, 1000, SG_CVAR_PERSISTENT);
    sg_sys_fmtsize(sg_sys_defaultsize,
                   info->default_width, info->default_height);
    sg_cvar_defstring(
        "video", "size", "Window size, e.g., 1920x1080 or 1080p",
        &sg_sys.vidsize, sg_sys_defaultsize, SG_CVAR_PERSISTENT);
    sg_cvar_defbool(
        "video", "hidpi", "Enable high DPI display support",
        &sg_sys.hidpi, 1, SG_CVAR_INITONLY | SG_CVAR_PERSISTENT);
}

void
sg_sys_draw(int width, int height, double time)
{
    static int last_width, last_height;
    static double time_ref;
    static int frame_count;
    double time_delta;

    if (width != last_width || height != last_height) {
        char buf[SG_SYS_SIZESZ];
        sg_sys_fmtsize(buf, width, height);
        sg_cvar_set_obj("video", "size", (union sg_cvar *) &sg_sys.vidsize,
                        buf, last_width != 0 ? SG_CVAR_PERSISTENT : 0);
        sg_sys.vidsize.flags &= ~SG_CVAR_MODIFIED;
        last_width = width;
        last_height = height;
    }

    frame_count++;
    time_delta = time - time_ref;
    if (time_delta > 1.0) {
        if (sg_sys.showfps.value) {
            sg_logf(SG_LOG_INFO, "FPS: %.0f", frame_count / time_delta);
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
sg_sys_postdraw(void)
{
    sg_timer_invoke();
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
