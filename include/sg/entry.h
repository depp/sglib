/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#ifndef SG_ENTRY_H
#define SG_ENTRY_H
/* Entry points: the layer that connects library code, game code, and
   platform code.  */
#include <stdarg.h>
#include "sg/defs.h"
#ifdef __cplusplus
extern "C" {
#endif
union sg_event;
struct sg_error;
struct sg_logger;

#define SG_GAME_ASPECT_SCALE 0x10000

struct sg_game_info {
    /* Minimum window size */
    int min_width, min_height;

    /* Default window size */
    int default_width, default_height;

    /* Minimum and maximum aspect ratio, multiplied by
       SG_GAME_ASPECT_SCALE (i.e., 16.16 fixed point) */
    int min_aspect, max_aspect;
};

/* ===== Game exports =====

   These are provided by the game code.  These should NOT be called
   directly by platform code, platform code should use the wrappers
   below.  */

/* Called once after the game launches.  No game entry points will be
   used before this is called.  */
void
sg_game_init(void);

/* Get information about the game.  */
void
sg_game_getinfo(struct sg_game_info *info);

/* Called for every event.  The OpenGL context may not be valid, you
   should not attempt to draw to the screen, load textures, etc.  */
void
sg_game_event(union sg_event *evt);

/* Draw the scene.  The OpenGL context will be valid.  The x, y,
   width, and height parameters specify the current viewport, the game
   should not draw outside this area.  The msecs parameter is the
   number of milliseconds since an arbitrary time.  Note that msecs
   overflows every 47 days.  */
void
sg_game_draw(int x, int y, int width, int height, unsigned msec);

/* Called at most once before the process exits.  */
void
sg_game_destroy(void);

/* ===== Platform exports =====

   These are called by game and library code.  Each platform must
   implement these.  */

/* Causes the game to exit.  Additional events or drawing commands may
   be issued before the game exits.  */
void
sg_platform_quit(void);

/* Signal a serious error.  */
void
sg_platform_failf(const char *fmt, ...);

/* Signal a serious error.  */
void
sg_platform_failv(const char *fmt, va_list ap);

/* Signal a serious error.  */
void
sg_platform_faile(struct sg_error *err);

/* Return immediately from the game code to the platform code and
   display the queued error message to the user.  This should only be
   used after a serious error has already been signaled using
   sg_platform_failf or sg_platform_faile.  */
__attribute__((noreturn))
void
sg_platform_return(void);

#ifdef __cplusplus
}
#endif
#endif
