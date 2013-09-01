/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "libpce/hashtable.h"
#include "libpce/thread.h"
#include "sg/aio.h"
#include "sg/dispatch.h"
#include "sg/error.h"
#include "sg/file.h"
#include "sg/log.h"
#include "sg/pixbuf.h"
#include "sg/texture.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

/* Global texture variable structure */
struct sg_texture_global {
    /* Lock for all the fields in this structure, as well as the
       `load` field in textures.  */
    struct pce_lock lock;

    /* Table of all textures indexed by path.  This allows two
       requests for the same texture to return the same object.  */
    struct pce_hashtable table;
};

/* Global texture variables.  */
static struct sg_texture_global sg_texture_global;

struct sg_texture_load {
    const char *path;
    int pathlen;
    struct sg_texture *tex;
    struct sg_aio_request *ioreq;
    struct sg_buffer *buf;
    struct sg_pixbuf pixbuf;
};

void
sg_texture_init(void)
{
    pce_lock_init(&sg_texture_global.lock);
}

/* Finish a texture loading operation.

   This frees the texture load structure.  In order to be safe, this
   should either be called with the lock held, or be called from the
   main thread.

   If err is not NULL and does not have domain SG_ERROR_CANCEL, then
   an error is logged indicating that texture loading failed.
   Otherwise, the texture loading operation was either completed
   successfully or canceled successfully.  This function does not
   distinguish between a canceled and a successful loading operation,
   so err can be NULL when the operation is canceled.  */
static void
sg_texture_finish(struct sg_texture_load *lp, struct sg_error *err)
{
    if (err && err->domain != &SG_ERROR_CANCEL) {
        sg_logerrf(sg_logger_get("rsrc"), LOG_ERROR,
                   err, "%s: texture failed to load", lp->path);
    }

    if (lp->tex)
        lp->tex->load = NULL;
    if (lp->buf)
        sg_buffer_decref(lp->buf);
    sg_pixbuf_destroy(&lp->pixbuf);
    free(lp);
    sg_dispatch_resource_adjust(-1);
}

/* Texture loading callback when IO operation completes */
static void
sg_texture_aiocb(void *cxt, struct sg_buffer *buf, struct sg_error *err);

/* Texture loading callback to decode file data */
static void
sg_texture_decodecb(void *cxt);

/* Texture loading callback to upload image data */
static void
sg_texture_uploadcb(void *cxt);

static void
sg_texture_aiocb(void *cxt, struct sg_buffer *buf, struct sg_error *err)
{
    struct sg_texture_load *lp = cxt;
    struct sg_texture_global *gp = &sg_texture_global;
    pce_lock_acquire(&gp->lock);
    lp->ioreq = NULL;

    if (!lp->tex || !buf) {
        sg_texture_finish(lp, err);
    } else {
        sg_buffer_incref(buf);
        lp->buf = buf;
        sg_dispatch_queue(0, lp, sg_texture_decodecb);
    }

    pce_lock_release(&gp->lock);
}

static void
sg_texture_decodecb(void *cxt)
{
    struct sg_texture_load *lp = cxt;
    struct sg_texture_global *gp = &sg_texture_global;
    struct sg_buffer *buf;
    struct sg_pixbuf pixbuf;
    struct sg_error *err = NULL;
    int r;

    pce_lock_acquire(&gp->lock);
    if (!lp->tex) {
        sg_texture_finish(lp, NULL);
    } else {
        buf = lp->buf;
        lp->buf = NULL;
        pce_lock_release(&gp->lock);

        sg_pixbuf_init(&pixbuf);
        r = sg_pixbuf_loadimage(&pixbuf, buf->data, buf->length, &err);
        sg_buffer_decref(buf);

        pce_lock_acquire(&gp->lock);
        if (!lp->tex || r) {
            sg_pixbuf_destroy(&pixbuf);
            sg_texture_finish(lp, err);
        } else {
            lp->pixbuf = pixbuf;
            sg_dispatch_sync_queue(
                SG_PRE_RENDER, 0, NULL,
                lp, sg_texture_uploadcb);
        }
    }
    pce_lock_release(&gp->lock);
}

struct sg_texture_fmt {
    GLenum ifmt, fmt, type;
};

static const struct sg_texture_fmt SG_TEXTURE_FMT[SG_PIXBUF_NFORMAT] = {
    { GL_LUMINANCE8, GL_LUMINANCE, GL_UNSIGNED_BYTE },
    { GL_LUMINANCE8_ALPHA8, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE },
    { GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE },
    { GL_RGB8, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8 },
    { GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE }
};

static void
sg_texture_uploadcb(void *cxt)
{
    struct sg_texture_load *lp = cxt;
    /* struct sg_texture_global *gp = &sg_texture_global; */
    struct sg_texture *tex;
    const struct sg_texture_fmt *ifo;
    GLint twidth;
    GLuint texnum;
    GLenum glerr;
    char errbuf[12];

    if (!lp->tex) {
        sg_texture_finish(lp, NULL);
        return;
    }

    ifo = &SG_TEXTURE_FMT[lp->pixbuf.format];
    glGenTextures(1, &texnum);
    glBindTexture(GL_TEXTURE_2D, texnum);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0,
                 ifo->ifmt, lp->pixbuf.pwidth, lp->pixbuf.pheight, 0,
                 ifo->fmt, ifo->type, lp->pixbuf.data);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0,
                             GL_TEXTURE_WIDTH, &twidth);

    glerr = glGetError();
    if (glerr) {
        /* FIXME: do we want to do more? */
        sg_logf(sg_logger_get("rsrc"), LOG_ERROR,
                "failed to upload texture: %s",
                sg_error_openglname(errbuf, glerr));
    }

    tex = lp->tex;
    tex->texnum = texnum;
    tex->iwidth = lp->pixbuf.iwidth;
    tex->iheight = lp->pixbuf.iheight;
    tex->twidth = lp->pixbuf.pwidth;
    tex->theight = lp->pixbuf.pheight;

    sg_texture_finish(lp, NULL);
}

void
sg_texture_free_(struct sg_texture *tp)
{
    struct sg_texture_global *gp = &sg_texture_global;
    struct sg_texture_load *lp;
    struct pce_hashtable_entry *ep;

    pce_lock_acquire(&gp->lock);

    /* Cancel any load in progress */
    lp = tp->load;
    if (lp) {
        if (lp->ioreq)
            sg_aio_cancel(lp->ioreq);
        lp->tex = NULL;
    }

    /* Remove texture from global table */
    ep = pce_hashtable_get(&gp->table, tp->path);
    assert(ep && ep->value == tp);
    pce_hashtable_erase(&gp->table, ep);

    pce_lock_release(&gp->lock);

    if (tp->texnum)
        glDeleteTextures(1, &tp->texnum);
    free(tp);
}

struct sg_texture *
sg_texture_file(const char *path, size_t pathlen,
                struct sg_error **err)
{
    struct sg_texture_global *gp = &sg_texture_global;
    struct pce_hashtable_entry *ep = NULL;
    struct sg_texture *tex = NULL;
    struct sg_texture_load *lp = NULL;
    struct sg_aio_request *ioreq;
    char npath[SG_MAX_PATH], *pp;
    int npathlen;

    npathlen = sg_path_norm(npath, path, pathlen, err);
    if (npathlen < 0)
        return NULL;

    pce_lock_acquire(&gp->lock);
    ep = pce_hashtable_insert(&gp->table, npath);
    if (!ep)
        goto nomem;

    tex = ep->value;
    if (tex) {
        tex->refcount += 1;
        pce_lock_release(&gp->lock);
        return tex;
    }

    tex = malloc(sizeof(*tex) + npathlen + 1);
    if (!tex)
        goto nomem;

    lp = malloc(sizeof(*lp) + npathlen + 1);
    if (!lp)
        goto nomem;

    ioreq = sg_aio_request(
        npath, npathlen, 0,
        SG_PIXBUF_IMAGE_EXTENSIONS,
        SG_PIXBUF_MAXFILESZ,
        lp, sg_texture_aiocb,
        err);
    if (!ioreq)
        goto error;

    pp = (char *) (lp + 1);
    memcpy(pp, npath, npathlen + 1);
    lp->path = pp;
    lp->pathlen = npathlen;
    lp->tex = tex;
    lp->ioreq = ioreq;
    lp->buf = NULL;
    sg_pixbuf_init(&lp->pixbuf);

    pp = (char *) (tex + 1);
    memcpy(pp, npath, npathlen + 1);
    tex->refcount = 1;
    tex->load = lp;
    tex->path = pp;
    tex->texnum = 0;
    tex->iwidth = 0;
    tex->iheight = 0;
    tex->twidth = 0;
    tex->theight = 0;

    ep->key = pp;
    ep->value = tex;

    sg_dispatch_resource_adjust(+1);
    pce_lock_release(&gp->lock);
    return tex;

nomem:
    sg_error_nomem(err);
    goto error;

error:
    if (ep)
        pce_hashtable_erase(&gp->table, ep);
    free(tex);
    free(lp);
    pce_lock_release(&gp->lock);
    return NULL;
}
