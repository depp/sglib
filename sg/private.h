/* Copyright 2013 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
struct sg_game_info;
union sg_event;

/* ===== Subsystem initialization ===== */

/* Initialize the asynchronous IO subsystem.  */
void
sg_aio_init(void);

/* Initialize the main audio system.  */
void
sg_audio_sys_init(void);

/* Initialize the timer, setting the current time to zero.  */
void
sg_clock_init(void);

/* Initialize asynchronous task dispatching system.  */
void
sg_dispatch_init(void);

/* Initialize logging subsystem.  */
void
sg_log_init(void);

/* Shut down logging system: close sockets, etc.  */
void
sg_log_term(void);

/* Initialize network subsystem.  Safe to call multiple times.
   Returns 1 if successful, 0 for failure.  */
int
sg_net_init(void);

/* Initialize the path subsystem.  */
void
sg_path_init(void);

/* ===== System exports =====

   These provide a thin layer around the game export functions, or
   provide a thin layer around platform functionality.  */

struct sg_sys_state {
    int width, height;
    unsigned status;
    unsigned tick;
    unsigned tick_offset;

    unsigned rec_ref;
    unsigned rec_numer;
    unsigned rec_denom;
    unsigned rec_next;
    unsigned rec_ct;
};

extern struct sg_sys_state sg_sst;

/* Initialize all library subsystems and the game.  This should be
   called after the command line arguments are parsed and passed to
   sg_cvar_addarg.  Does not initialize audio output.  */
void
sg_sys_init(void);

/* Get information about the game.  Details not provided by the game
   are filled in with sensible defaults.  */
void
sg_sys_getinfo(struct sg_game_info *info);

/* Called for every event.  */
void
sg_sys_event(union sg_event *evt);

/* Draw the scene.  The OpenGL context must be valid.  */
void
sg_sys_draw(void);

/* Perform any cleanup necessary before the process exits.  */
void
sg_sys_destroy(void);
