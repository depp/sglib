#define MODEL_MAXSIZE (256 * 1024)

#include "error.h"
#include "lfile.h"
#include "model.h"
#include "opengl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char const MODEL_HDR[16] = "Egg3D Model\0\4\3\2\1";

/* An array of model chunks is terminated with NULL data.  */
struct sg_modelchunk {
    unsigned char name[4];
    unsigned length;
    const void *data;
};

static int
sg_model_readchunks(struct sg_modelchunk *chunk, unsigned maxchunk,
                    const unsigned char *ptr, size_t len)
{
    unsigned n = 0;
    size_t pos = 0;
    while (len - pos >= 8) {
        if (n >= maxchunk - 1)
            return -1;
        memcpy(&chunk[n].name, ptr + pos, 4);
        memcpy(&chunk[n].length, ptr + pos + 4, 4);
        if (chunk[n].length > (len - pos - 12))
            return -1;
        chunk[n].data = ptr + pos + 8;
        pos += (8 + chunk[n].length + 3) & ~3U;
        n += 1;
    }
    if (len - pos != 4 || memcmp(ptr + pos, "END ", 4))
        return -1;
    memset(chunk[n].name, '\0', 4);
    chunk[n].length = 0;
    chunk[n].data = NULL;
    return 0;
}

static void
sg_model_zero(struct sg_model *mp)
{
    mp->scale = 1.0;
    mp->vtype = 0;
    mp->vcount = 0;
    mp->vdata = NULL;
    mp->ttype = 0;
    mp->tcount = 0;
    mp->tdata = NULL;
    mp->ltype = 0;
    mp->lcount = 0;
    mp->ldata = NULL;
}

static void
sg_model_unload(struct sg_model *mp)
{
    if (mp->vdata)
        free(mp->vdata);
    if (mp->tdata)
        free(mp->tdata);
    if (mp->ldata)
        free(mp->ldata);
    sg_model_zero(mp);
}

static int
sg_model_load2(struct sg_model *mp, struct sg_buffer *fbuf,
               struct sg_error **err)
{
    struct sg_modelchunk chunk[8], *cp;
    void *p;
    int r;

    if (fbuf->length < 16 || memcmp(fbuf->data, MODEL_HDR, 16))
        goto data_error;

    r = sg_model_readchunks(
        chunk, 8, (unsigned char *) fbuf->data + 16, fbuf->length - 16);
    if (r)
        goto data_error;

    for (cp = chunk; cp->data; ++cp) {
        if (!memcmp(cp->name, "SclF", 4)) {
            if (cp->length != 4)
                return -1;
            mp->scale = *(const float *) cp->data;
        } else if (!memcmp(cp->name, "VrtS", 4)) {
            if (cp->length % 6)
                goto data_error;
            p = malloc(cp->length);
            if (!p)
                goto nomem;
            memcpy(p, cp->data, cp->length);
            mp->vtype = GL_SHORT;
            mp->vcount = cp->length / 6;
            mp->vdata = p;
        } else if (!memcmp(cp->name, "LinS", 4)) {
            // FIXME easy to do buffer overflow
            // with indices out of range
            if (cp->length % 4)
                goto data_error;
            p = malloc(cp->length);
            if (!p)
                goto nomem;
            memcpy(p, cp->data, cp->length);
            mp->ltype = GL_UNSIGNED_SHORT;
            mp->lcount = cp->length / 4;
            mp->ldata = p;
        } else if (!memcmp(cp->name, "TriS", 4)) {
            if (cp->length % 6)
                goto data_error;
            p = malloc(cp->length);
            if (!p)
                goto nomem;
            memcpy(p, cp->data, cp->length);
            mp->ttype = GL_UNSIGNED_SHORT;
            mp->tcount = cp->length / 6;
            mp->tdata = p;
        } else
            return -1;
    }

    return 0;

data_error:
    sg_error_data(err, "egg3d");
    return -1;

nomem:
    sg_error_nomem(err);
    return -1;
}

int
sg_model_load(struct sg_model *mp, struct sg_error **err)
{
    struct sg_buffer fbuf;
    size_t maxsize = MODEL_MAXSIZE;
    int ret, r;

    sg_model_unload(mp);
    r = sg_file_get(mp->r.name, 0, &fbuf, maxsize, err);
    if (r)
        goto error;
    r = sg_model_load2(mp, &fbuf, err);
    sg_buffer_destroy(&fbuf);
    if (r)
        goto error;

    fprintf(stderr, "loaded %s\n", mp->r.name);
    ret = 0;
    goto done;

error:
    fprintf(stderr, "error loading %s\n", mp->r.name);
    sg_model_unload(mp);
    ret = -1;
    goto done;

done:
    return ret;
}

struct sg_model *
sg_model_new(const char *path, struct sg_error **err)
{
    struct sg_model *mp;
    size_t l;
    char *npath;
    int r;
    mp = (struct sg_model *)
        sg_resource_find(SG_RSRC_MODEL, path);
    if (mp) {
        sg_resource_incref(&mp->r);
        return mp;
    }
    l = strlen(path);
    mp = malloc(sizeof(*mp) + l + 1);
    if (!mp) {
        sg_error_nomem(err);
        return NULL;
    }
    npath = (char *) (mp + 1);
    mp->r.type = SG_RSRC_MODEL;
    mp->r.refcount = 1;
    mp->r.flags = 0;
    mp->r.name = npath;
    sg_model_zero(mp);
    memcpy(npath, path, l + 1);
    r = sg_resource_register(&mp->r, err);
    if (r) {
        free(mp);
        return NULL;
    }
    return mp;
}

void
sg_model_draw(struct sg_model *mp)
{
    if (!mp->vdata)
        return;

    glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT);
    glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
    glPushMatrix();
    glScaled(mp->scale, mp->scale, mp->scale);

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, mp->vtype, 0, mp->vdata);
    if (mp->tcount) {
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(1.0f, 1.0f);
        glColor3ub(128, 128, 128);
        glDrawElements(GL_TRIANGLES, mp->tcount * 3,
                       mp->ttype, mp->tdata);
    }
    if (mp->lcount) {
        glColor3ub(255, 255, 255);
        glDrawElements(GL_LINES, mp->lcount * 2,
                       mp->ltype, mp->ldata);
    }

    glPopMatrix();
    glPopClientAttrib();
    glPopAttrib();
}
