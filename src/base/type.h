#ifndef BASE_TYPE_H
#define BASE_TYPE_H
#ifdef __cplusplus
extern "C" {
#endif

/* A layout object consists of text, styles, and all data necessary to
   draw text to the screen.  A layout object will manage the necessary
   OpenGL textures.  When drawn to the screen, it will draw objects
   with an alpha texture.  (You must set up the blend mode
   yourself.)  */
struct sg_layout;

/* Information about the text style.  */
struct sg_style;

struct sg_layout *
sg_layout_new(void);

void
sg_layout_incref(struct sg_layout *lp);

void
sg_layout_decref(struct sg_layout *lp);

/* Set the text in the layout to the given string.  The string must be
   UTF-8 encoded.  */
void
sg_layout_settext(struct sg_layout *lp, const char *text, unsigned length);

/* Draw the layout to the OpenGL context.  */
void
sg_layout_draw(struct sg_layout *lp);

/* Draw the border and origin.  Used for debugging layouts.  */
void
sg_layout_drawmarks(struct sg_layout *lp);

/* Set the width of the layout.  Lines that extend beyond this width
   are wrapped or truncated.  A negative width indicates unlimited
   width, which is the default.  Note that the actual pixel bounds may
   extend beyond this.  */
void
sg_layout_setwidth(struct sg_layout *lp, float width);

/* Set the base style of the entire layout.  This uses the current
   value of the style; the style can be modified or freed afterwards
   without affecting the layout.  */
void
sg_layout_setstyle(struct sg_layout *lp, struct sg_style *sp);

struct sg_style *
sg_style_new(void);

void
sg_style_incref(struct sg_style *sp);

void
sg_style_decref(struct sg_style *sp);

void
sg_style_setfamily(struct sg_style *sp, const char *family);

void
sg_style_setsize(struct sg_style *sp, float size);

#ifdef __cplusplus
}
#endif
#endif
