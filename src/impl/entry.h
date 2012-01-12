#ifndef IMPL_ENTRY_H
#define IMPL_ENTRY_H
/* Entry points: the layer that connects library code, game code, and
   platform code.  */
#include <stdarg.h>
#ifdef __cplusplus
extern "C" {
#endif
union sg_event;
struct sg_error;

/* ===== Game exports =====

   These are provided by the game code.  These should NOT be called
   directly by platform code, platform code should use the wrappers
   below.  */

/* Called once after the game launches.  No game entry points will be
   used before this is called.  */
void
sg_game_init(void);

/* Get the default width and height of the game window.  Platforms are
   free to ignore this information.  */
void
sg_game_getsize(int *width, int *height);

/* Called for every event.  The OpenGL context may not be valid, you
   should not attempt to draw to the screen, load textures, etc.  */
void
sg_game_event(union sg_event *evt);

/* Draw the scene.  The OpenGL context will be valid.  */
void
sg_game_draw(unsigned msecs);

/* Called at most once before the process exits.  */
void
sg_game_destroy(void);

/* ===== System exports =====

   These provide a thin layer around the game export functions.  This
   provides some simple common functionality.  */

/* Initialize all library subsystems and the game.  This should be
   called after the command line arguments are parsed and passed to
   sg_cvar_addarg.  */
void
sg_sys_init(void);

/* Get the default width and height of the game window.  */
void
sg_sys_getsize(int *width, int *height);

/* Called for every event.  */
void
sg_sys_event(union sg_event *evt);

/* Draw the scene.  The OpenGL context must be valid.  */
void
sg_sys_draw(void);

/* Perform any cleanup necessary before the process exits.  */
void
sg_sys_destroy(void);

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
