#ifndef IMPL_TYPE_H
#define IMPL_TYPE_H
#ifdef __cplusplus
extern "C" {
#endif

struct sg_layout;

struct sg_layout *
sg_layout_new(void);

void
sg_layout_incref(struct sg_layout *lp);

void
sg_layout_decref(struct sg_layout *lp);

void
sg_layout_settext(struct sg_layout *lp, const char *text, unsigned length);

void
sg_layout_draw(struct sg_layout *lp);

#ifdef __cplusplus
}
#endif
#endif
