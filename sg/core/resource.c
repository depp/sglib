/* Copyright 2012 Dietrich Epp <depp@zdome.net> */
/*
  A quick tour of the resource manager:

  Some changes to resources get made through the change queue.  The
  queue is managed with a lock so it can be accessed from any thread.
  This same lock manages access to the "action" field of every
  resource, which is 1 if the resource is currently in the queue and 0
  otherwise.  This prevents resources from being added to the queue
  multiple times.

  Every frame, sg_resource_updateall is called which goes through the
  queue, changes resource states, and frees unreferenced resources.
  When a resource is freed, it may release other resources, so the
  queue is cleared repeatedly until it stays empty.
*/
#include "libpce/strbuf.h"
#include "libpce/thread.h"
#include "sg/error.h"
#include "sg/log.h"
#include "sg/resource.h"
#include <stdlib.h>

const struct sg_error_domain SG_RESOURCE_CANCEL = { "resource_cancel" };

/* Background task status.  */
typedef enum {
    /* No change in background task.  Used for resources with zero
       refcount that should be freed.  */
    SG_RSRCI_NONE,

    /* Background task finished successfully.  Call
       'load_finished'.  */
    SG_RSRCI_LOADED,

    /* Background task failed.  Call 'load_finished'.  */
    SG_RSRCI_FAILED,

    /* Background task detected cancellation before running.  Do not
       call 'load_finished'.  */
    SG_RSRCI_CANCELED_1,

    /* Background task detected cancellation while running.  Call
       'load_finished'.  */
    SG_RSRCI_CANCELED_2,
} sg_resource_itype_t;

struct sg_resource_item {
    struct sg_resource *rsrc;
    struct sg_error *err;
    void *result;
    sg_resource_itype_t type;
};

struct sg_resource_set {
    struct pce_lock lock;
    struct sg_resource_item *items;
    unsigned icount;
    unsigned ialloc;
    struct sg_logger *logger;
};

static struct sg_resource_set sg_resource_set;

static void
sg_resource_queue(struct sg_resource_item *it)
{
    struct sg_resource_item *ip, *ie;
    struct sg_resource *rs = it->rsrc;
    unsigned nc, na;

    pce_lock_acquire(&sg_resource_set.lock);
    ip = sg_resource_set.items;
    if (rs->action) {
        ie = ip + sg_resource_set.icount;
        for (; ip != ie; ++ip)
            if (ip->rsrc == rs)
                break;
        assert(ip != ie);
        if (it->type == SG_RSRCI_NONE)
            (void)0;
        else if (ip->type == SG_RSRCI_NONE)
            memcpy(ip, it, sizeof(*it));
        else
            assert(0);
    } else {
        rs->action = 1;
        nc = sg_resource_set.icount;
        na = sg_resource_set.ialloc;
        if (nc >= na) {
            na = na ? na * 2 : 4;
            assert(na > 0);
            ip = realloc(ip, sizeof(*ip) * na);
            if (!ip)
                abort();
            sg_resource_set.items = ip;
            sg_resource_set.ialloc = na;
        }
        memcpy(&ip[nc], it, sizeof(*it));
        sg_resource_set.icount = nc + 1;
    }
    pce_lock_release(&sg_resource_set.lock);
}

static void
sg_resource_load(void *ptr)
{
    struct sg_resource_item it;
    struct sg_resource *rs = ptr;
    struct sg_error *err = NULL;

    it.rsrc = rs;
    if (rs->state == SG_RSRC_PENDING_CANCEL) {
        it.err = NULL;
        it.result = NULL;
        it.type = SG_RSRCI_CANCELED_1;
    } else {
        it.result = rs->type->load(rs, &err);
        if (!err) {
            it.err = NULL;
            it.type = SG_RSRCI_LOADED;
        } else if (err->domain == &SG_RESOURCE_CANCEL) {
            sg_error_clear(&err);
            it.err = NULL;
            it.type = SG_RSRCI_CANCELED_2;
        } else {
            it.err = err;
            it.type = SG_RSRCI_FAILED;
        }
    }

    sg_resource_queue(&it);
}

static void
sg_resource_getname(struct sg_resource *rs, struct pce_strbuf *buf,
                    size_t *namelen)
{
    if (*namelen) {
        pce_strbuf_setlen(buf, *namelen);
    } else {
        pce_strbuf_clear(buf);
        rs->type->get_name(rs, buf);
        *namelen = pce_strbuf_len(buf);
    }
}

void
sg_resource_init(void)
{
    sg_resource_set.logger = sg_logger_get("rsrc");
    pce_lock_init(&sg_resource_set.lock);
}

void
sg_resource_canceled(struct sg_error **err)
{
    sg_error_sets(err, &SG_RESOURCE_CANCEL, 0, NULL);
}

void
sg_resource_updateall(void)
{
    struct sg_resource_item *ia, *ip, *ie;
    struct sg_resource *rs;
    struct pce_strbuf buf;
    const struct sg_resource_type *rt;
    size_t namelen;
    struct sg_logger *logger = sg_resource_set.logger;
    struct sg_error *err;
    const char *action;
    sg_log_level_t level;
    void *result;

    pce_strbuf_init(&buf, 0);
    while (1) {
        pce_lock_acquire(&sg_resource_set.lock);
        ia = sg_resource_set.items;
        if (!ia) {
            pce_lock_release(&sg_resource_set.lock);
            break;
        }
        ie = ia + sg_resource_set.icount;
        sg_resource_set.items = NULL;
        sg_resource_set.icount = 0;
        sg_resource_set.ialloc = 0;
        for (ip = ia; ip != ie; ++ip) {
            ip->rsrc->action = 0;
            ip->rsrc->refcount += 1;
        }
        pce_lock_release(&sg_resource_set.lock);

        for (ip = ia; ip != ie; ++ip) {
            rs = ip->rsrc;
            rt = rs->type;
            result = ip->result;
            namelen = 0;
            action = NULL;

            switch (ip->type) {
            case SG_RSRCI_NONE:
                break;

            case SG_RSRCI_LOADED:
                rs->state = SG_RSRC_LOADED;
                rt->load_finished(rs, result);
                action = "loaded";
                break;

            case SG_RSRCI_FAILED:
                rs->state = SG_RSRC_FAILED;
                rt->load_finished(rs, result);
                err = ip->err;
                if (LOG_ERROR > logger->level) {
                    sg_resource_getname(rs, &buf, &namelen);
                    pce_strbuf_puts(&buf, ": failed to load: ");
                    pce_strbuf_puts(&buf, err->msg);
                    pce_strbuf_puts(&buf, " (");
                    pce_strbuf_puts(&buf, err->domain->name);
                    if (err->code) {
                        pce_strbuf_putc(&buf, ' ');
                        pce_strbuf_printf(&buf, "%ld", err->code);
                    }
                    pce_strbuf_putc(&buf, ')');
                    sg_logs(logger, LOG_ERROR, buf.s);
                }
                sg_error_clear(&ip->err);
                break;

            case SG_RSRCI_CANCELED_1:
                rs->state = SG_RSRC_UNLOADED;
                action = "canceled";
                break;

            case SG_RSRCI_CANCELED_2:
                rs->state = SG_RSRC_UNLOADED;
                rt->load_finished(rs, result);
                action = "canceled";
                break;
            }

            if (action && LOG_INFO > logger->level) {
                sg_resource_getname(rs, &buf, &namelen);
                pce_strbuf_puts(&buf, ": ");
                pce_strbuf_puts(&buf, action);
                sg_logs(logger, LOG_INFO, buf.s);
            }

            rs->refcount -= 1;
            if (rs->refcount) {
                sg_resource_update(rs);
            } else {
                action = NULL;
                level = LOG_INFO;
                switch (rs->state) {
                case SG_RSRC_LOADED:
                    action = "unloaded";
                    break;

                case SG_RSRC_INITIAL:
                case SG_RSRC_FAILED:
                case SG_RSRC_UNLOADED:
                    action = "freed";
                    level = LOG_DEBUG;
                    break;

                case SG_RSRC_LOADING:
                    rs->state = SG_RSRC_PENDING_CANCEL;
                    break;

                default:
                    break;
                }

                if (action) {
                    if (level > logger->level) {
                        sg_resource_getname(rs, &buf, &namelen);
                        pce_strbuf_puts(&buf, ": ");
                        pce_strbuf_puts(&buf, action);
                        sg_logs(logger, level, buf.s);
                    }
                    rs->type->free(rs);
                }
            }
        }

        free(ia);
    }
    pce_strbuf_destroy(&buf);
}

void
sg_resource_update(struct sg_resource *rs)
{
    struct sg_resource_item it;
    if (!rs->refcount) {
        switch (rs->state) {
        case SG_RSRC_INITIAL:
        case SG_RSRC_UNLOADED:
        case SG_RSRC_LOADED:
        case SG_RSRC_FAILED:
            it.rsrc = rs;
            it.err = NULL;
            it.result = NULL;
            it.type = SG_RSRCI_NONE;
            sg_resource_queue(&it);
            break;

        case SG_RSRC_LOADING:
            rs->state = SG_RSRC_PENDING_CANCEL;
            break;

        case SG_RSRC_PENDING_CANCEL:
            break;
        }
    } else {
        switch (rs->state) {
        case SG_RSRC_INITIAL:
        case SG_RSRC_UNLOADED:
            rs->state = SG_RSRC_LOADING;
            sg_dispatch_queue(0, rs, sg_resource_load);
            break;

        case SG_RSRC_FAILED:
        case SG_RSRC_LOADED:
        case SG_RSRC_LOADING:
            break;

        case SG_RSRC_PENDING_CANCEL:
            rs->state = SG_RSRC_LOADING;
            break;
        }
    }
}

void
sg_resource_incref(struct sg_resource *rs)
{
    rs->refcount += 1;
    sg_resource_update(rs);
}

void
sg_resource_decref(struct sg_resource *rs)
{
    rs->refcount -= 1;
    sg_resource_update(rs);
}
