#define MODEL_MAXSIZE (256 * 1024)

#include "error.h"
#include "file.h"
#include "log.h"
#include "model.h"
#include "opengl.h"
#include <stdlib.h>
#include <string.h>

static const short CUBE_VERTEX[8][3] = {
    { -1, -1, -1 }, { -1, -1,  1 }, { -1,  1, -1 }, { -1,  1,  1 },
    {  1, -1, -1 }, {  1, -1,  1 }, {  1,  1, -1 }, {  1,  1,  1 }
};

static const unsigned short CUBE_TRI[12][3] = {
    { 1, 3, 2 }, { 2, 0, 1 },
    { 0, 2, 6 }, { 6, 4, 0 },
    { 4, 6, 7 }, { 7, 5, 4 },
    { 1, 5, 7 }, { 7, 3, 1 },
    { 2, 3, 7 }, { 7, 6, 2 },
    { 0, 4, 5 }, { 5, 1, 0 }
};

static const unsigned short CUBE_LINE[12][2] = {
    { 0, 1 }, { 2, 3 }, { 4, 5 }, { 6, 7 },
    { 0, 2 }, { 1, 3 }, { 4, 6 }, { 5, 7 },
    { 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 }
};

static struct sg_logger *sg_model_logger;

static void
sg_model_cube(struct sg_model *mp)
{
    mp->flags = SG_MODEL_STATIC;
    mp->scale = 1.0;
    mp->vtype = GL_SHORT;
    mp->vcount = 8;
    mp->vdata = (short *) CUBE_VERTEX;
    mp->ttype = GL_UNSIGNED_SHORT;
    mp->tcount = 12;
    mp->tdata = (unsigned short *) CUBE_TRI;
    mp->ltype = GL_UNSIGNED_SHORT;
    mp->lcount = 12;
    mp->ldata = (unsigned short *) CUBE_LINE;
}

static const short PYRAMID_VERTEX[5][3] = {
    { -1, -1, -1 }, { -1,  1, -1 }, {  1, -1, -1 }, {  1,  1, -1 },
    {  0,  0,  1 }
};

static const unsigned short PYRAMID_TRI[6][3] = {
    { 0, 1, 3 }, { 3, 2, 0 },
    { 0, 4, 1 }, { 1, 4, 3 },
    { 3, 4, 2 }, { 2, 4, 0 }
};

static const unsigned short PYRAMID_LINE[8][2] = {
    { 0, 1 }, { 2, 3 }, { 0, 2 }, { 1, 3 },
    { 0, 4 }, { 1, 4 }, { 2, 4 }, { 3, 4 }
};

static void
sg_model_pyramid(struct sg_model *mp)
{
    mp->flags = SG_MODEL_STATIC;
    mp->scale = 1.0;
    mp->vtype = GL_SHORT;
    mp->vcount = 5;
    mp->vdata = (short *) PYRAMID_VERTEX;
    mp->ttype = GL_UNSIGNED_SHORT;
    mp->tcount = 6;
    mp->tdata = (unsigned short *) PYRAMID_TRI;
    mp->ltype = GL_UNSIGNED_SHORT;
    mp->lcount = 8;
    mp->ldata = (unsigned short *) PYRAMID_LINE;
}

struct sg_model *
sg_model_static(sg_model_static_t which, struct sg_error **err)
{
    void (*init)(struct sg_model *) = NULL;
    const char *name = NULL;
    struct sg_model *mp;
    int r;

    switch (which) {
    case SG_MODEL_CUBE:
        name = "static:cube";
        init = sg_model_cube;
        break;

    case SG_MODEL_PYRAMID:
        name = "static:pyramid";
        init = sg_model_pyramid;
        break;
    }

    if (!name)
        abort();

    mp = (struct sg_model *) sg_resource_find(SG_RSRC_MODEL, name);
    if (mp) {
        sg_resource_incref(&mp->r);
        return mp;
    }
    mp = malloc(sizeof(*mp));
    if (!mp) {
        sg_error_nomem(err);
        return NULL;
    }
    mp->r.type = SG_RSRC_MODEL;
    mp->r.refcount = 1;
    mp->r.flags = 0;
    mp->r.name = (char *) name;
    init(mp);
    r = sg_resource_register(&mp->r, err);
    if (r) {
        free(mp);
        return NULL;
    }
    return mp;
}

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
    mp->flags = 0;
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
    if (!(mp->flags & SG_MODEL_STATIC)) {
        if (mp->vdata)
            free(mp->vdata);
        if (mp->tdata)
            free(mp->tdata);
        if (mp->ldata)
            free(mp->ldata);
    }
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

    if (!sg_model_logger)
        sg_model_logger = sg_logger_get("model");

    if (!(mp->flags & SG_MODEL_STATIC)) {
        sg_model_unload(mp);
        r = sg_file_get(mp->r.name, strlen(mp->r.name),
                        0, &fbuf, maxsize, err);
        if (r)
            goto error;
        r = sg_model_load2(mp, &fbuf, err);
        sg_buffer_destroy(&fbuf);
        if (r)
            goto error;
    }

    sg_logf(sg_model_logger, LOG_INFO,
            "loaded %s", mp->r.name);
    ret = 0;
    goto done;

error:
    sg_logf(sg_model_logger, LOG_ERROR,
            "erorr loading %s", mp->r.name);
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
