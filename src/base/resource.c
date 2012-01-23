#include "dict.h"
#include "error.h"
#include "log.h"
#include "model.h"
#include "resource.h"
#include "texture.h"
#include <assert.h>
#include <stdlib.h>

#define RTYPE_COUNT ((int) SG_RSRC_MODEL + 1)

struct dict sg_resources[RTYPE_COUNT];

static int
sg_resource_istype(sg_resource_type_t t)
{
    return (int) t >= 0 && (int) t < RTYPE_COUNT;
}

struct sg_resource *
sg_resource_find(sg_resource_type_t type, const char *name)
{
    struct dict_entry *e;
    if (!sg_resource_istype(type))
        return NULL;
    e = dict_get(&sg_resources[(int) type], name);
    return e ? e->value : NULL;
}

int
sg_resource_register(struct sg_resource *rs, struct sg_error **err)
{
    struct dict_entry *e;
    if (!sg_resource_istype(rs->type)) {
        abort(); /* FIXME: error */
        return -1;
    }
    e = dict_insert(&sg_resources[(int) rs->type], rs->name);
    if (!e) {
        sg_error_nomem(err);
        return -1;
    }
    if (e->value) {
        abort(); /* FIXME: error */
        return -1;
    }
    e->value = rs;
    return 0;
}

static void
sg_resource_load(struct sg_resource *rs)
{
    struct sg_error *err = NULL;
    int r = 0;

    switch (rs->type) {
    case SG_RSRC_TEXTURE:
        r = sg_texture_load((struct sg_texture *) rs, &err);
        break;

    case SG_RSRC_MODEL:
        r = sg_model_load((struct sg_model *) rs, &err);
        break;

    default:
        abort();
        break;
    }

    if (r) {
        if (err) {
            sg_logf(sg_logger_get(NULL), LOG_ERROR,
                    "%s: %s (%s %ld)", rs->name, err->msg,
                    err->domain->name, err->code);
        } else {
            sg_logf(sg_logger_get(NULL), LOG_ERROR,
                    "%s: unknown error", rs->name);
        }
        sg_error_clear(&err);
    }
    rs->flags |= SG_RSRC_LOADED;
}

void
sg_resource_loadall(void)
{
    struct dict *d;
    struct dict_entry *p, *e;
    struct sg_resource *rs;
    int type;
    for (type = 0; type < RTYPE_COUNT; ++type) {
        d = &sg_resources[type];
        p = d->contents;
        e = p + d->capacity;
        for (; p != e; ++p) {
            rs = p->value;
            if (rs && (rs->flags & SG_RSRC_LOADED) == 0)
                sg_resource_load(rs);
        }
    }
}

void
sg_resource_dirtytype(sg_resource_type_t type)
{
    struct dict *d;
    struct dict_entry *p, *e;
    struct sg_resource *rs;
    assert(sg_resource_istype(type));
    d = &sg_resources[(int) type];
    p = d->contents;
    e = p + d->capacity;
    for (; p != e; ++p) {
        rs = p->value;
        if (rs)
            rs->flags &= ~SG_RSRC_LOADED;
    }
}
