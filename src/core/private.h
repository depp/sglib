/* Copyright 2013 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include <stddef.h>
struct sg_game_info;
union sg_event;

/* ===== Subsystem initialization ===== */

/* The initialization funcitons are called in this order, with the
   exception of sg_net_init(), which may be called at any time after
   the logging system is initialized.  */

/* Initialize logging subsystem.  This is called first so other
   subsystems can log errors.  */
void
sg_log_init(void);

/* Initialize the path subsystem.  */
void
sg_path_init(void);

/* Initialize the timer, setting the current time to zero.  */
void
sg_clock_init(void);

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

/* Initialize all library subsystems and the game.  This should be
   called after the command line arguments are parsed and passed to
   sg_cvar_addarg.  */
void
sg_sys_init(void);

/* Get information about the game.  Details not provided by the game
   are filled in with sensible defaults.  This should be called by
   platform code instead of sg_game_getinfo().  */
void
sg_sys_getinfo(struct sg_game_info *info);

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

#endif
