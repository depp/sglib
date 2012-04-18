#include "opengl.h"
#include "pixbuf.h"
#include "type.h"
#include "type_impl.h"
#include <stdlib.h>
#include <string.h>

struct sg_layout *
sg_layout_new(void)
{
    struct sg_layout *lp;
    lp = malloc(sizeof(*lp));
    if (!lp)
        abort();
    lp->refcount = 1;
    lp->text = NULL;
    lp->textlen = 0;
    lp->texnum = 0;
    lp->width = -1.0f;
    lp->family = NULL;
    lp->size = 16.0f;

    return lp;
}

static void
sg_layout_free(struct sg_layout *lp)
{
    if (lp->texnum)
        glDeleteTextures(1, &lp->texnum);
    free(lp);
}

void
sg_layout_incref(struct sg_layout *lp)
{
    if (lp)
        lp->refcount++;
}

void
sg_layout_decref(struct sg_layout *lp)
{
    if (lp && --lp->refcount == 0)
        sg_layout_free(lp);
}

void
sg_layout_settext(struct sg_layout *lp, const char *text, unsigned length)
{
    char *tp;
    tp = malloc(length + 1);
    if (!tp)
        abort();
    memcpy(tp, text, length);
    tp[length] = '\0';
    if (lp->text)
        free(lp->text);
    lp->text = tp;
    lp->textlen = length;
}

static void
sg_layout_load(struct sg_layout *lp)
{
    struct sg_layout_impl *li;
    struct sg_layout_bounds b;
    struct sg_pixbuf pbuf;
    struct sg_error *err = NULL;
    int r, xoff, yoff;
    float xscale, yscale;

    li = sg_layout_impl_new(lp);

    sg_layout_impl_calcbounds(li, &b);

    lp->vx0 = (float) (b.ibounds.x - b.x);
    lp->vx1 = (float) (b.ibounds.x - b.x + b.ibounds.width);
    lp->vy0 = (float) (b.ibounds.y - b.y);
    lp->vy1 = (float) (b.ibounds.y - b.y + b.ibounds.height);

    sg_pixbuf_init(&pbuf);
    r = sg_pixbuf_set(&pbuf, SG_Y, b.ibounds.width, b.ibounds.height, &err);
    if (r)
        abort();
    r = sg_pixbuf_calloc(&pbuf, &err);
    if (r)
        abort();

    xoff = (pbuf.pwidth - b.ibounds.width) >> 1;
    yoff = (pbuf.pheight - b.ibounds.height) >> 1;
    xscale = 1.0f / pbuf.pwidth;
    yscale = 1.0f / pbuf.pheight;
    lp->tx0 = xscale * xoff;
    lp->tx1 = xscale * (xoff + b.ibounds.width);
    lp->ty0 = yscale * (pbuf.pheight - yoff);
    lp->ty1 = yscale * (pbuf.pheight - yoff - b.ibounds.height);

    sg_layout_impl_render(li, &pbuf, xoff - b.ibounds.x, yoff - b.ibounds.y);

    sg_layout_impl_free(li);

    if (!lp->texnum)
        glGenTextures(1, &lp->texnum);
    glPushAttrib(GL_ENABLE_BIT);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, lp->texnum);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0,
                 GL_ALPHA, pbuf.pwidth, pbuf.pheight, 0,
                 GL_ALPHA, GL_UNSIGNED_BYTE, pbuf.data);
    glPopAttrib();
}

void
sg_layout_draw(struct sg_layout *lp)
{
    if (!lp->texnum)
        sg_layout_load(lp);

    if (0) {
        glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
        glDisable(GL_TEXTURE_2D);
        glColor4ub(0, 0, 255, 128);
        glBegin(GL_TRIANGLE_STRIP);
        glVertex2f(lp->vx0, lp->vy0);
        glVertex2f(lp->vx1, lp->vy0);
        glVertex2f(lp->vx0, lp->vy1);
        glVertex2f(lp->vx1, lp->vy1);
        glEnd();
        glPopAttrib();
    }

    glPushAttrib(GL_ENABLE_BIT);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, lp->texnum);
    glBegin(GL_TRIANGLE_STRIP);
    glTexCoord2f(lp->tx0, lp->ty0); glVertex2f(lp->vx0, lp->vy0);
    glTexCoord2f(lp->tx1, lp->ty0); glVertex2f(lp->vx1, lp->vy0);
    glTexCoord2f(lp->tx0, lp->ty1); glVertex2f(lp->vx0, lp->vy1);
    glTexCoord2f(lp->tx1, lp->ty1); glVertex2f(lp->vx1, lp->vy1);
    glEnd();
    glPopAttrib();
}

void
sg_layout_setstyle(struct sg_layout *lp, struct sg_style *sp)
{
    char *f;
    size_t s;
    if (sp->family) {
        s = strlen(sp->family);
        f = malloc(s + 1);
        if (!f)
            abort();
        memcpy(f, sp->family, s + 1);
    } else {
        f = NULL;
    }
    if (lp->family)
        free(lp->family);
    lp->family = f;
    lp->size = sp->size;
}

void
sg_layout_setwidth(struct sg_layout *lp, float width)
{
    lp->width = width;
}

struct sg_style *
sg_style_new(void)
{
    struct sg_style *sp;
    sp = malloc(sizeof(*sp));
    if (!sp)
        abort();
    sp->refcount = 1;
    sp->family = NULL;
    sp->size = 16.0f;
    return sp;
}

static void
sg_style_free(struct sg_style *sp)
{
    if (sp->family)
        free(sp->family);
    free(sp);
}

void
sg_style_incref(struct sg_style *sp)
{
    if (sp)
        sp->refcount++;
}

void
sg_style_decref(struct sg_style *sp)
{
    if (sp && --sp->refcount == 0)
        sg_style_free(sp);
}

void
sg_style_setfamily(struct sg_style *sp, const char *family)
{
    char *f;
    size_t s;
    if (family) {
        s = strlen(family);
        f = malloc(s + 1);
        if (!f)
            abort();
        memcpy(f, family, s + 1);
    } else {
        f = NULL;
    }
    if (sp->family)
        free(sp->family);
    sp->family = f;
}

void
sg_style_setsize(struct sg_style *sp, float size)
{
    sp->size = size;
}

