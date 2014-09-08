/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "log_impl.h"
#include "sg/thread.h"
#include "sg/clock.h"
#include "sg/cvar.h"
#include "sg/error.h"
#include "sg/log.h"
#include "private.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOG_BUFSZ 256
#define MAX_LISTENERS 4
#define LOG_INHERIT ((sg_log_level_t) -1)

struct sg_logger_obj {
    struct sg_logger head;
    char *name;
    size_t namelen;
    struct sg_logger_obj *next;
};

static struct sg_logger_obj sg_logger_root;
static struct sg_log_listener *sg_listeners[4];
static struct sg_lock sg_logger_lock;

void
sg_log_listen(struct sg_log_listener *listener)
{
    int i;
    for (i = 0; i < 4; ++i) {
        if (!sg_listeners[i]) {
            sg_listeners[i] = listener;
            return;
        }
    }
    sg_logs(&sg_logger_root.head, SG_LOG_WARN,
            "Too many log listeners, log listener dropped.");
}

static sg_log_level_t
sg_logger_conflevel(const char *name)
{
    const char *v;
    int r;
    r = sg_cvar_gets("log.level", name, &v);
    if (r) {
        if (!strcmp(v, "debug"))
            return SG_LOG_DEBUG;
        if (!strcmp(v, "info"))
            return SG_LOG_INFO;
        if (!strcmp(v, "warn"))
            return SG_LOG_WARN;
        if (!strcmp(v, "error"))
            return SG_LOG_ERROR;
    }
    return LOG_INHERIT;
}

void
sg_log_init(void)
{
    sg_log_level_t level = sg_logger_conflevel("root");
    sg_lock_init(&sg_logger_lock);
    sg_logger_root.head.level = level == LOG_INHERIT ? SG_LOG_WARN : level;
    sg_log_console_init();
    sg_log_network_init();
}

void
sg_log_term(void)
{
    int i;
    for (i = 0; i < MAX_LISTENERS; ++i)
        if (sg_listeners[i])
            sg_listeners[i]->destroy(sg_listeners[i]);
}

static struct sg_logger_obj *
sg_logger_new(const char *name, size_t namelen)
{
    struct sg_logger_obj *np;
    sg_log_level_t level;
    np = malloc(sizeof(*np) + namelen + 1);
    if (!np)
        abort();
    level = sg_logger_conflevel(name);
    np->head.level = level == LOG_INHERIT ? sg_logger_root.head.level : level;
    np->name = (char *) (np + 1);
    np->namelen = namelen;
    np->next = sg_logger_root.next;
    sg_logger_root.next = np;
    memcpy(np->name, name, namelen + 1);
    return np;
}

struct sg_logger *
sg_logger_get(const char *name)
{
    struct sg_logger_obj *np;
    size_t len;

    if (!name)
        return &sg_logger_root.head;

    len = strlen(name);

    sg_lock_acquire(&sg_logger_lock);
    for (np = &sg_logger_root; np; np = np->next) {
        if (np->namelen == len && !memcmp(np->name, name, len))
            break;
    }
    if (!np)
        np = sg_logger_new(name, len);
    sg_lock_release(&sg_logger_lock);

    return &np->head;
}

static const char SG_LOGLEVEL[4][6] = {
    "DEBUG", "INFO", "WARN", "ERROR"
};

static void
sg_dologmem(struct sg_logger *logger, sg_log_level_t level,
            const char *msg, size_t len)
{
    struct sg_log_listener *p;
    struct sg_logger_obj *lp = (struct sg_logger_obj *) logger;
    struct sg_log_msg m;
    const char *levelname;
    char date[SG_DATE_LEN];
    int datelen, levellen;
    int i;

    datelen = sg_clock_getdate(date, 0);
    if ((int) level < 0)
        level = 0;
    else if ((int) level > 3)
        level = 3;
    levelname = SG_LOGLEVEL[(int) level];
    levellen = (int) strlen(levelname);

    m.date = date;
    m.datelen = datelen;
    m.level = levelname;
    m.levellen = levellen;
    m.name = lp->name;
    m.namelen = lp->namelen;
    m.msg = msg;
    m.msglen = len;
    m.levelval = level;

    sg_lock_acquire(&sg_logger_lock);
    for (i = 0; i < MAX_LISTENERS; ++i) {
        p = sg_listeners[i];
        if (p) {
            sg_listeners[i] = NULL;
            p->msg(p, &m);
            sg_listeners[i] = p;
        }
    }
    sg_lock_release(&sg_logger_lock);
}

/* Be careful because the MSC version of the sprintf family behaves
   differently. */
#ifdef _MSC_VER
#define vsnprintf _vsnprintf
#define snprintf _snprintf
#endif

static void
sg_dologv(struct sg_logger *logger, sg_log_level_t level,
          struct sg_error *err, const char *msg, va_list ap)
{
    char buf[LOG_BUFSZ];
    int r, s;
    r = vsnprintf(buf, sizeof(buf), msg, ap);
    if (r < 0)
        r = 0;
    else if ((size_t) r >= sizeof(buf))
        r = sizeof(buf) - 1;
    if (err) {
        if (err->code) {
            s = snprintf(
                buf + r, sizeof(buf) - r,
                ": %s (%s %ld)", err->msg, err->domain->name, err->code);
        } else {
            s = snprintf(
                buf + r, sizeof(buf) - r,
                ": %s (%s)", err->msg, err->domain->name);
        }
        if (s > 0) {
            r += s;
            if ((size_t) r >= sizeof(buf))
                r = sizeof(buf) - 1;
        }
    }
    sg_dologmem(logger, level, buf, r);
}

void
sg_logs(struct sg_logger *logger, sg_log_level_t level,
        const char *msg)
{
    if (level < logger->level)
        return;
    sg_dologmem(logger, level, msg, strlen(msg));
}

void
sg_logf(struct sg_logger *logger, sg_log_level_t level,
        const char *msg, ...)
{
    va_list ap;
    if (level < logger->level)
        return;
    va_start(ap, msg);
    sg_dologv(logger, level, NULL, msg, ap);
    va_end(ap);
}

void
sg_logv(struct sg_logger *logger, sg_log_level_t level,
        const char *msg, va_list ap)
{
    if (level < logger->level)
        return;
    sg_dologv(logger, level, NULL, msg, ap);
}

void
sg_logerrs(struct sg_logger *logger, sg_log_level_t level,
           struct sg_error *err, const char *msg)
{
    sg_logerrf(logger, level, err, "%s", msg);
}

void
sg_logerrf(struct sg_logger *logger, sg_log_level_t level,
           struct sg_error *err, const char *msg, ...)
{
    va_list ap;
    if (level < logger->level)
        return;
    va_start(ap, msg);
    sg_dologv(logger, level, err, msg, ap);
    va_end(ap);
}

void
sg_logerrv(struct sg_logger *logger, sg_log_level_t level,
           struct sg_error *err, const char *msg, va_list ap)
{
    if (level < logger->level)
        return;
    sg_dologv(logger, level, err, msg, ap);
}
