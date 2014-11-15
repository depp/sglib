/* Copyright 2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "private.h"
#include "sg/error.h"
#include "sg/log.h"
#include "sg/opengl.h"
#include "sg/pack.h"
#include "sg/pixbuf.h"
#include "sg/type.h"
#include <assert.h>
#include <math.h>
#include <stdlib.h>

static void
sg_font_free(struct sg_font *fp)
{
    struct sg_font *p, **prev;
    struct sg_typeface *t;
    t = fp->typeface;
    prev = &t->font;
    p = *prev;
    while (1) {
        if (p == fp) {
            *prev = fp->next;
            if (prev == &t->font)
                sg_typeface_decref(t);
            break;
        } else if (!p) {
            abort();
        } else {
            prev = &p->next;
            p = *prev;
        }
    }
    glDeleteTextures(1, &fp->texture);
    free(fp);
}

void
sg_font_incref(struct sg_font *fp)
{
    fp->refcount++;
}

void
sg_font_decref(struct sg_font *fp)
{
    if (!--fp->refcount)
        sg_font_free(fp);
}

#include <stdio.h>

struct sg_font *
sg_font_new(struct sg_typeface *tp, float size, struct sg_error **err)
{
    struct sg_font *fp = NULL;
    struct sg_font_glyph *glyph;
    struct sg_pack_rect *grect = NULL;
    struct sg_pack_size psize, maxsize;
    struct sg_pixbuf pixbuf;
    FT_Error ferr;
    FT_Face face = tp->face;
    FT_GlyphSlot slot;
    int isize, i, nglyph, r, w, h, x, y, irb, orb;
    const unsigned char *ip;
    unsigned char *op;
    GLuint texture;

    pixbuf.data = NULL;

    isize = floorf((size * 64.0f) + 0.5f);
    if (isize < 1 || isize > 64 * 1024) {
        sg_error_invalid(err, __FUNCTION__, "size");
        return NULL;
    }

    for (fp = tp->font; fp; fp = fp->next) {
        if (fp->size == isize) {
            fp->refcount++;
            return fp;
        }
    }

    ferr = FT_Set_Char_Size(face, 0, isize, 72, 72);
    if (ferr)
        goto freetype_error;

    nglyph = face->num_glyphs;
    grect = malloc(sizeof(*grect) * nglyph);
    for (i = 0; i < nglyph; i++) {
        ferr = FT_Load_Glyph(face, i, FT_LOAD_TARGET_NORMAL);
        if (ferr)
            goto freetype_error;
        slot = face->glyph;
        grect[i].w = slot->metrics.width >> 6;
        grect[i].h = slot->metrics.height >> 6;
    }
    maxsize.width = 1024;
    maxsize.height = 1024;
    r = sg_pack(grect, nglyph, &psize, &maxsize, err);
    if (r <= 0) {
        if (r == 0) {
            sg_error_sets(err, &SG_ERROR_GENERIC, 0,
                          "could not pack font glyphs");
        }
        goto error;
    }
    sg_logf(SG_LOG_INFO,
            "%s @%.2f: %d glyphs packed in %dx%d rect\n",
            tp->path, (double) isize * (1.0 / 64.0),
            nglyph, psize.width, psize.height);
    r = sg_pixbuf_calloc(&pixbuf, SG_R, psize.width, psize.height, err);
    if (r)
        goto error;
    orb = pixbuf.rowbytes;
    fp = malloc(sizeof(*fp) + sizeof(*glyph) * nglyph);
    if (!fp) {
        sg_error_nomem(err);
        goto error;
    }
    glyph = (struct sg_font_glyph *) (fp + 1);
    for (i = 0; i < nglyph; i++) {
        ferr = FT_Load_Glyph(face, i, FT_LOAD_TARGET_NORMAL);
        slot = face->glyph;
        if (ferr)
            goto freetype_error;
        ferr = FT_Render_Glyph(slot, FT_RENDER_MODE_NORMAL);
        if (ferr)
            goto freetype_error;
        w = grect[i].w;
        h = grect[i].h;
        assert(w == slot->bitmap.width);
        assert(h == slot->bitmap.rows);
        glyph[i].advance = slot->advance.x >> 6;
        if (w == 0 || h == 0) {
            glyph[i].w = 0;
            glyph[i].h = 0;
            glyph[i].x = 0;
            glyph[i].y = 0;
            glyph[i].bx = 0;
            glyph[i].by = 0;
        } else {
            glyph[i].w = w;
            glyph[i].h = h;
            glyph[i].x = grect[i].x;
            glyph[i].y = grect[i].y;
            glyph[i].bx = slot->bitmap_left;
            glyph[i].by = slot->bitmap_top;
            ip = slot->bitmap.buffer;
            irb = slot->bitmap.pitch;
            op = (unsigned char *) pixbuf.data +
                grect[i].y * orb + grect[i].x;
            for (y = 0; y < h; y++)
                for (x = 0; x < w; x++)
                    op[y * orb + x] = ip[y * irb + x];
        }
    }

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    sg_pixbuf_texture(&pixbuf);
    glBindTexture(GL_TEXTURE_2D, 0);

    fp->refcount = 1;
    fp->size = isize;
    fp->ascender = (int) face->size->metrics.ascender >> 6;
    fp->descender = (int) face->size->metrics.descender >> 6;
    fp->typeface = tp;
    fp->next = tp->font;
    fp->glyph = glyph;
    fp->glyphcount = nglyph;
    fp->texture = texture;
    fp->texscale[0] = 1.0f / (float) pixbuf.width;
    fp->texscale[1] = 1.0f / (float) pixbuf.height;
    if (tp->font == NULL)
        tp->refcount++;

    free(pixbuf.data);
    free(grect);
    return fp;

freetype_error:
    sg_error_freetype(err, ferr);
    goto error;

error:
    free(fp);
    free(pixbuf.data);
    free(grect);
    return NULL;
}
