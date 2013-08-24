/* Copyright 2013 Dietrich Epp <depp@zdome.net> */
#include "libpce/hashtable.h"
#include "libpce/thread.h"
#include "sg/aio.h"
#include "sg/dispatch.h"
#include "sg/error.h"
#include "sg/file.h"
#include "sg/log.h"
#include "sg/shader.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define SG_SHADER_MAXFILESZ (1024 * 32)

struct sg_shader_global {
    /* Lock for all the fields in this structure, as well as the load
       field in shaders.  */
    struct pce_lock lock;

    /* Table of all shaders indexed by path.  The paths are augmented
       with query strings that indicate the shader type.  */
    struct pce_hashtable table;
};

static struct sg_shader_global sg_shader_global;

void
sg_shader_init(void)
{
    pce_lock_init(&sg_shader_global.lock);
    pce_hashtable_init(&sg_shader_global.table);
}

struct sg_shader_type {
    GLenum type;
    char name[12];
};

static const struct sg_shader_type SG_SHADER_TYPES[] = {
    { GL_VERTEX_SHADER, "vertex" },
    { GL_FRAGMENT_SHADER, "fragment" },
    { GL_GEOMETRY_SHADER, "geometry" }
};

struct sg_shader_load {
    const char *path;
    int pathlen;
    struct sg_shader *sp;
    struct sg_aio_request *ioreq;
    struct sg_buffer *buf;
};

static void
sg_shader_finish(struct sg_shader_load *lp, struct sg_error *err)
{
    if (err && err->domain != &SG_ERROR_CANCEL) {
        sg_logerrf(sg_logger_get("shader"), LOG_ERROR,
                   err, "%s: shader failed to load", lp->path);
    }

    if (lp->sp)
        lp->sp->load = NULL;
    if (lp->buf)
        sg_buffer_decref(lp->buf);
    free(lp);
}

static void
sg_shader_uploadcb(void *cxt);

static void
sg_shader_aiocb(void *cxt, struct sg_buffer *buf, struct sg_error *err)
{
    struct sg_shader_load *lp = cxt;
    struct sg_shader_global *gp = &sg_shader_global;
    pce_lock_acquire(&gp->lock);
    lp->ioreq = NULL;

    if (!lp->sp || !buf) {
        sg_shader_finish(lp, err);
    } else {
        sg_buffer_incref(buf);
        lp->buf = buf;
        sg_dispatch_sync_queue(
            SG_PRE_RENDER, 0, NULL,
            lp, sg_shader_uploadcb);
    }
    pce_lock_release(&gp->lock);
}

static void
sg_shader_uploadcb(void *cxt)
{
    struct sg_shader_load *lp = cxt;
    const char *darr[1];
    GLint larr[1], flag, loglen;
    GLuint shader;
    struct sg_logger *log;
    char *errlog;
    struct sg_error *err = NULL;

    if (!lp->sp) {
        sg_shader_finish(lp, NULL);
        return;
    }

    shader = glCreateShader(lp->sp->type);
    darr[0] = lp->buf->data;
    larr[0] = (GLint) lp->buf->length;
    glShaderSource(shader, 1, darr, larr);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &flag);
    if (flag) {
        lp->sp->shader = shader;
        sg_shader_finish(lp, NULL);
        return;
    }

    log = sg_logger_get("shader");
    sg_logf(log, LOG_ERROR, "%s: compilation failed", lp->path);
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &loglen);
    if (loglen > 0) {
        errlog = malloc(loglen);
        if (!errlog) {
            sg_error_nomem(&err);
        } else {
            glGetShaderInfoLog(shader, loglen, NULL, errlog);
            sg_logs(log, LOG_ERROR, errlog);
            free(errlog);
        }
    }
    glDeleteShader(shader);
    sg_shader_finish(lp, err);
    sg_error_clear(&err);
}

void
sg_shader_free_(struct sg_shader *sp)
{
    struct sg_shader_global *gp = &sg_shader_global;
    struct sg_shader_load *lp;
    struct pce_hashtable_entry *ep;

    pce_lock_acquire(&gp->lock);

    /* Cancel any load in progress */
    lp = sp->load;
    if (lp) {
        if (lp->ioreq)
            sg_aio_cancel(lp->ioreq);
        lp->sp = NULL;
    }

    /* Remove shader from global cache */
    ep = pce_hashtable_get(&gp->table, sp->path);
    assert(ep && ep->value == sp);
    pce_hashtable_erase(&gp->table, ep);

    pce_lock_release(&gp->lock);

    if (sp->shader)
        glDeleteShader(sp->shader);
    free(sp);
}

struct sg_shader *
sg_shader_file(const char *path, size_t pathlen, GLenum type,
               struct sg_error **err)
{
    struct sg_shader_global *gp = &sg_shader_global;
    struct pce_hashtable_entry *ep = NULL;
    struct sg_shader *sp = NULL;
    struct sg_shader_load *lp = NULL;
    struct sg_aio_request *ioreq;
    char npath[SG_MAX_PATH + 16], *pp;
    const char *tnam;
    int npathlen, npathlenfull;
    size_t i, tlen;

    tnam = NULL;
    for (i = 0; i < sizeof(SG_SHADER_TYPES) / sizeof(*SG_SHADER_TYPES); i++) {
        if (SG_SHADER_TYPES[i].type == type) {
            tnam = SG_SHADER_TYPES[i].name;
            break;
        }
    }
    if (!tnam)
        abort();

    npathlen = sg_path_norm(npath, path, pathlen, err);
    if (npathlen < 0)
        return NULL;
    memcpy(npath + npathlen, "?type=", 6);
    tlen = strlen(tnam);
    memcpy(npath + npathlen + 6, tnam, tlen + 1);
    npathlenfull = tlen + 6 + tlen;

    pce_lock_acquire(&gp->lock);
    ep = pce_hashtable_insert(&gp->table, npath);
    if (!ep)
        goto nomem;

    sp = ep->value;
    if (sp) {
        sp->refcount += 1;
        pce_lock_release(&gp->lock);
        assert(sp->type == type);
        return sp;
    }

    sp = malloc(sizeof(*sp) + npathlenfull + 1);
    if (!sp)
        goto nomem;

    lp = malloc(sizeof(*lp) + npathlen + 1);
    if (!lp)
        goto nomem;

    ioreq = sg_aio_request(
        npath, npathlen, 0,
        "glsl",
        SG_SHADER_MAXFILESZ,
        lp, sg_shader_aiocb,
        err);
    if (!ioreq)
        goto error;

    pp = (char *) (lp + 1);
    memcpy(pp, npath, npathlen);
    pp[npathlen] = '\0';
    lp->path = pp;
    lp->pathlen = npathlen;
    lp->sp = sp;
    lp->ioreq = ioreq;
    lp->buf = NULL;

    pp = (char *) (sp + 1);
    memcpy(pp, npath, npathlenfull + 1);
    sp->refcount = 1;
    sp->shader = 0;
    sp->type = type;
    sp->load = lp;
    sp->path = pp;

    ep->key = pp;
    ep->value = sp;

    pce_lock_release(&gp->lock);
    return sp;

nomem:
    sg_error_nomem(err);
    goto error;

error:
    if (ep)
        pce_hashtable_erase(&gp->table, ep);
    free(sp);
    free(lp);
    pce_lock_release(&gp->lock);
    return NULL;
}
