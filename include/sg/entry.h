/* Copyright 2012-2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#ifndef SG_ENTRY_H
#define SG_ENTRY_H
#include <stdarg.h>
#include "sg/defs.h"
#ifdef __cplusplus
extern "C" {
#endif
union sg_event;
struct sg_error;

/**
 * @file sg/entry.h
 *
 * @brief Game entry points.
 *
 * This layer connects the library code with the game code.  This file
 * defines functions both in the game code (which must be implemented
 * by the game) and functions in the library code.
 */

/**
 * @brief Game flags, returned in the ::sg_game_info structure.
 */
enum {
    /**
     * @brief Allow high resolution windows.
     */
    SG_GAME_ALLOW_HIDPI = 01,

    /**
     * @brief Allow the window to be resized.
     */
    SG_GAME_ALLOW_RESIZE = 02
};

/**
 * @brief Game startup information.
 */
struct sg_game_info {
    /** @brief The minimum window width.  */
    int min_width;
    /** @brief The minimum window height.  */
    int min_height;

    /** @brief The suggested default window width.  */
    int default_width;
    /** @brief The suggested default window height.  */
    int default_height;

    /** @brief The minimum window aspect ratio, width divided by
        height.  */
    double min_aspect;
    /** @brief The maximum window aspect ratio, width divided by
        height.  */
    double max_aspect;

    /** @brief Game flags.  */
    unsigned flags;

    /** @brief The name of the game.  */
    const char *name;
};

/**
 * @brief Handle game startup.
 *
 * This is a game entry point, and must be implemented.  This function
 * will be called exactly once, before any other entry points.  The
 * OpenGL context will not be valid.
 */
void
sg_game_init(void);

/**
 * @brief Handle request for game startup information.
 *
 * This is a game entry point, and must be implemented.  The OpenGL
 * context will not be valid.
 *
 * @param info When called, this contains the default startup
 * information.  This may be modified or left unmodified.
 */
void
sg_game_getinfo(struct sg_game_info *info);

/**
 * @brief Handle an event.
 *
 * This is a game entry point, and must be implemented.  The OpenGL
 * context will not be valid except for certain event types.
 *
 * @brief evt The event to handle.
 */
void
sg_game_event(union sg_event *evt);

/**
 * @brief Handle screen redraw.
 *
 * This is a game entry point, and must be implemented.  OpenGL
 * functions may be used.
 *
 * @param width The viewport width.
 * @param height The viewport height.
 * @param time The current time, in seconds, since startup.
 */
void
sg_game_draw(int width, int height, double time);

/**
 * @brief Handle game shutdown.
 *
 * This is a game entry point, and must be implemented.  The OpenGL
 * context will not be valid.  This function may be called at most
 * once, before the program shuts down.  No other entry points will be
 * called after this one.
 */
void
sg_game_destroy(void);

/**
 * @brief Quit the program.
 */
void
sg_sys_quit(void);

/**
 * @brief Display the given error message and exit the program
 * immediately.
 *
 * @param msg The message to display, a NUL-terminated UTF-8 string.
 */
SG_ATTR_NORETURN
void
sg_sys_abort(const char *msg);

/**
 * @brief Display the given error message and exit the program
 * immediately.
 *
 * @param msg The message to display, a NUL-terminated UTF-8 string.
 */
SG_ATTR_NORETURN
SG_ATTR_FORMAT(printf, 1, 2)
void
sg_sys_abortf(const char *msg, ...);

/**
 * @brief Display the given error message and exit the program
 * immediately.
 *
 * @param msg The message to display, a NUL-terminated UTF-8 string.
 */
SG_ATTR_NORETURN
void
sg_sys_abortv(const char *msg, va_list ap);

/**
 * @brief Enable or disable mouse capture.
 *
 * @param enabled Whether mouse capture should be enabled.
 */
void
sg_sys_capturemouse(int enabled);

#ifdef __cplusplus
}
#endif
#endif
