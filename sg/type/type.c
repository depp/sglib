/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "impl.h"
#include "sg/opengl.h"
#include "sg/pixbuf.h"
#include "sg/type.h"
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
    lp->boxalign = 0;
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
    struct sg_pixbuf pbuf;
    struct sg_error *err = NULL;
    int r, xoff, yoff;
    float xscale, yscale;

    li = sg_layout_impl_new(lp);

    sg_layout_impl_calcbounds(li, &lp->bounds);

    sg_pixbuf_init(&pbuf);
    r = sg_pixbuf_set(
        &pbuf, SG_Y,
        lp->bounds.pixel.width,
        lp->bounds.pixel.height,
        &err);
    if (r)
        abort();
    r = sg_pixbuf_calloc(&pbuf, &err);
    if (r)
        abort();

    xoff = (pbuf.pwidth - lp->bounds.pixel.width) >> 1;
    yoff = (pbuf.pheight - lp->bounds.pixel.height) >> 1;
    xscale = 1.0f / pbuf.pwidth;
    yscale = 1.0f / pbuf.pheight;
    lp->tx0 = xscale * xoff;
    lp->tx1 = xscale * (xoff + lp->bounds.pixel.width);
    lp->ty0 = yscale * (pbuf.pheight - yoff);
    lp->ty1 = yscale * (pbuf.pheight - yoff - lp->bounds.pixel.height);

    sg_layout_impl_render(
        li, &pbuf,
        xoff - lp->bounds.pixel.x,
        yoff - lp->bounds.pixel.y);

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

static struct sg_layout_point
sg_layout_getoff(struct sg_layout *lp)
{
    struct sg_layout_point p;

    switch (lp->boxalign & SG_VALIGN_MASK) {
    default:
    case SG_VALIGN_ORIGIN:
        p.y = lp->bounds.y;
        break;

    case SG_VALIGN_TOP:
        p.y = lp->bounds.logical.y + lp->bounds.logical.height;
        break;

    case SG_VALIGN_CENTER:
        p.y = lp->bounds.logical.y + (lp->bounds.logical.height >> 1);
        break;

    case SG_VALIGN_BOTTOM:
        p.y = lp->bounds.logical.y;
        break;
    }

    switch (lp->boxalign & SG_HALIGN_MASK) {
    default:
    case SG_HALIGN_ORIGIN:
        p.x = lp->bounds.x;
        break;

    case SG_HALIGN_LEFT:
        p.x = lp->bounds.logical.x;
        break;

    case SG_HALIGN_CENTER:
        p.x = lp->bounds.logical.x + (lp->bounds.logical.width >> 1);
        break;

    case SG_HALIGN_RIGHT:
        p.x = lp->bounds.logical.x + lp->bounds.logical.width;
        break;
    }

    return p;
}

void
sg_layout_draw(struct sg_layout *lp)
{
    struct sg_layout_point orig;
    float tx0, tx1, ty0, ty1;
    float vx0, vx1, vy0, vy1;
    if (!lp->texnum)
        sg_layout_load(lp);

    orig = sg_layout_getoff(lp);
    tx0 = lp->tx0; tx1 = lp->tx1; ty0 = lp->ty0; ty1 = lp->ty1;
    vx0 = (float) (lp->bounds.pixel.x - orig.x);
    vx1 = (float) (lp->bounds.pixel.x - orig.x + lp->bounds.pixel.width);
    vy0 = (float) (lp->bounds.pixel.y - orig.y);
    vy1 = (float) (lp->bounds.pixel.y - orig.y + lp->bounds.pixel.height);

    glPushAttrib(GL_ENABLE_BIT);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, lp->texnum);
    glBegin(GL_TRIANGLE_STRIP);
    glTexCoord2f(tx0, ty0); glVertex2f(vx0, vy0);
    glTexCoord2f(tx1, ty0); glVertex2f(vx1, vy0);
    glTexCoord2f(tx0, ty1); glVertex2f(vx0, vy1);
    glTexCoord2f(tx1, ty1); glVertex2f(vx1, vy1);
    glEnd();
    glPopAttrib();
}

void
sg_layout_drawmarks(struct sg_layout *lp)
{
    struct sg_layout_point orig;
    float vx0, vx1, vy0, vy1;
    if (!lp->texnum)
        sg_layout_load(lp);

    orig = sg_layout_getoff(lp);
    vx0 = (float) (lp->bounds.pixel.x - orig.x);
    vx1 = (float) (lp->bounds.pixel.x - orig.x + lp->bounds.pixel.width);
    vy0 = (float) (lp->bounds.pixel.y - orig.y);
    vy1 = (float) (lp->bounds.pixel.y - orig.y + lp->bounds.pixel.height);

    glBegin(GL_LINE_LOOP);
    glVertex2f(vx0 + 0.5f, vy0 + 0.5f);
    glVertex2f(vx1 - 0.5f, vy0 + 0.5f);
    glVertex2f(vx1 - 0.5f, vy1 - 0.5f);
    glVertex2f(vx0 + 0.5f, vy1 - 0.5f);
    glEnd();

    vx0 = (float) (lp->bounds.logical.x - orig.x);
    vx1 = (float) (lp->bounds.logical.x - orig.x + lp->bounds.logical.width);
    vy0 = (float) (lp->bounds.logical.y - orig.y);
    vy1 = (float) (lp->bounds.logical.y - orig.y + lp->bounds.logical.height);

    glBegin(GL_LINE_LOOP);
    glVertex2f(vx0 + 0.5f, vy0 + 0.5f);
    glVertex2f(vx1 - 0.5f, vy0 + 0.5f);
    glVertex2f(vx1 - 0.5f, vy1 - 0.5f);
    glVertex2f(vx0 + 0.5f, vy1 - 0.5f);
    glEnd();

    glBegin(GL_LINE_LOOP);
    glVertex2f( 4.5f, 4.5f);
    glVertex2f(-4.5f, 4.5f);
    glVertex2f(-4.5f,-4.5f);
    glVertex2f( 4.5f,-4.5f);
    glEnd();
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

void
sg_layout_setboxalign(struct sg_layout *lp, int align)
{
    lp->boxalign = align;
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
