/* Copyright 2013-2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include <stddef.h>
#include "sg/cvar.h"
struct sg_game_info;
union sg_event;

extern const struct sg_game_info sg_game_info_defaults;

/* Common globals */
struct sg_sys {
    struct sg_cvar_bool showfps;
    struct sg_cvar_int vsync;
    struct sg_cvar_int maxfps;
};

extern struct sg_sys sg_sys;

/* ===== Subsystem initialization ===== */

/* The initialization funcitons are called in this order, with the
   exception of sg_net_init(), which may be called at any time after
   the logging system is initialized.  */

/* Initialize the timer, setting the current time to zero.  */
void
sg_clock_init(void);

/* Initialize logging subsystem.  This is called first so other
   subsystems can log errors.  */
void
sg_log_init(void);

/* Initialize the path subsystem.  */
void
sg_path_init(void);

/* Initialize the main audio system.  */
void
sg_mixer_init(void);

/* Shut down logging system: close sockets, etc.  */
void
sg_log_term(void);

/* Initialize network subsystem.  Safe to call multiple times.
   Returns 1 if successful, 0 for failure.  */
int
sg_net_init(void);

/* ===== System exports =====

   These provide a thin layer around the game export functions, or
   provide a thin layer around platform functionality.  */

/* Initialize all library subsystems and the game.  The arguments
   should not include the executable name, i.e., if called from
   main(), call sg_sys_init(argc-1, argv+1).  */
void
sg_sys_init(int argc, char **argv);

/* Get information about the game.  Details not provided by the game
   are filled in with sensible defaults.  This should be called by
   platform code instead of sg_game_getinfo().  */
void
sg_sys_getinfo(struct sg_game_info *info);

/* Draw the current frame.  */
void
sg_sys_draw(int width, int height, double time);

/* Perform any cleanup necessary before the process exits.  */
void
sg_sys_destroy(void);

#if defined(_WIN32)

/* Convert a UTF-8 string to UTF-16, alloctaing the destination buffer.
   Returns 0 for success.  You must free the buffer afterwards.  The
   destlen pointer may be NULL.  The dest pointer must not be NULL.  */
int
sg_wchar_from_utf8(wchar_t **dest, int *destlen,
                   const char *src, size_t srclen);

/* Reverse of the above conversion. */
int
sg_utf8_from_wchar(
    char **dest,
    size_t *destlen,
    const wchar_t *src,
    size_t srclen);

#endif
