#ifndef IMPL_ENTRY_H
#define IMPL_ENTRY_H
#include <stdarg.h>
#ifdef __cplusplus
extern "C" {
#endif
union sg_event;
struct sg_error;

/* ===== Game entry points =====

   These are called by the platform and library code.  Each game must
   implement these.  */

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

/* Called once before the game quits, unless a serious error is
   signaled such as with sg_platform_failf.  No game entry points will
   be used after this is called.  */
void
sg_game_destroy(void);

/* ===== Platform entry points =====

   These are called by the game code.  Each platform must implement
   these.  */

/* Causes the game to exit.  Additional events or drawing commands may
   be issued before the game exits.  */
void
sg_platform_quit(void);

/* Signal a serious error from the game code.  No additional game
   entry points will be called, the message will be displayed, and the
   game will quit.  */
void
sg_platform_failf(const char *fmt, ...);

/* Signal a serious error from the game code.  No additional game
   entry points will be called, the message will be displayed, and the
   game will quit.  */
void
sg_platform_failv(const char *fmt, va_list ap);

/* Signal a serious error from the game code.  No additional game
   entry points will be called, the error will be displayed, and the
   game will quit.  */
void
sg_platform_faile(struct sg_error *err);

/* Return immediately from the game code to the platform code and
   display the queued error message to the user immediately.  This
   should only be used after a serious error has already been
   signaled using sg_platform_failf or sg_platform_faile.  */
__attribute__((noreturn))
void
sg_platform_return(void);

#ifdef __cplusplus
}
#endif
#endif
