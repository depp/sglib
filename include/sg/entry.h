/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#ifndef SG_ENTRY_H
#define SG_ENTRY_H
/* Entry points: the layer that connects library code, game code, and
   platform code.  */
#include <stdarg.h>
#include "libpce/attribute.h"
#include "sg/audio.h"
#ifdef __cplusplus
extern "C" {
#endif
union sg_event;
struct sg_error;
struct sg_logger;

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
 * @param msec The current time, in milliseconds since startup.
 */
void
sg_game_draw(int width, int height, unsigned msec);

/**
 * @brief Get audio data.
 *
 * This is a game entry point, and must be implemented.  The OpenGL
 * context will not be valid.
 *
 * @param buffer The audio buffer to fill.  The buffer size is taken
 * from the audio initialization event.  The buffer format is always
 * interleaved stereo with 32-bit floating pont values.
 *
 * @param The current time, in milliseconds since startup.
 *
 * @return A result code for the audio stream.
 */
sg_audio_result_t
sg_game_audio(float *buffer, int msec);

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
 * @brief Start the audio system.
 *
 * If successful, this will send an audio initialization event, then
 * start calling the ::sg_game_audio callback.  The audio system may
 * be started on another thread and this function may return before
 * the audio system is fully initialized.
 *
 * @return 0 if the audio system was started, nonzero if the audio
 * system could not be started.
 */
int
sg_sys_audio_start(void);

/**
 * @brief Stop the audio system.
 *
 * This will wait for the audio system to shut down, which requires
 * handling the audio stop event.
 */
void
sg_sys_audio_stop(void);

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
PCE_ATTR_NORETURN
void
sg_sys_abort(const char *msg);

/**
 * @brief Display the given error message and exit the program
 * immediately.
 *
 * @param msg The message to display, a NUL-terminated UTF-8 string.
 */
PCE_ATTR_NORETURN
PCE_ATTR_FORMAT(printf, 1, 2)
void
sg_sys_abortf(const char *msg, ...);

/**
 * @brief Display the given error message and exit the program
 * immediately.
 *
 * @param msg The message to display, a NUL-terminated UTF-8 string.
 */
PCE_ATTR_NORETURN
void
sg_sys_abortv(const char *msg, va_list ap);

#ifdef __cplusplus
}
#endif
#endif
