/* Copyright 2012-2014 Dietrich Epp.
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
    sg_logs(SG_LOG_WARN, "Too many log listeners, log listener dropped.");
}

void
sg_log_init(void)
{
    char date[SG_DATE_LEN];

    sg_lock_init(&sg_logger_lock);
    sg_log_console_init();
    sg_log_network_init();

    sg_clock_getdate(date, 0);
    sg_logf(SG_LOG_INFO, "Startup %s", date);
}

void
sg_log_term(void)
{
    int i;
    for (i = 0; i < MAX_LISTENERS; ++i)
        if (sg_listeners[i])
            sg_listeners[i]->destroy(sg_listeners[i]);
}

static const char SG_LOGLEVEL[4][6] = {
    "DEBUG", "INFO", "WARN", "ERROR"
};

static void
sg_dologmem(sg_log_level_t level, const char *msg, size_t len)
{
    struct sg_log_listener *p;
    struct sg_log_msg m;
    const char *levelname;
    char time[32];
    int timelen, levellen;
    int i;

#if defined _WIN32
    _snprintf_s(time, sizeof(time), _TRUNCATE, "%9.3f", sg_clock_get());
#else
    snprintf(time, sizeof(time), "%9.3f", sg_clock_get());
#endif
    timelen = (int) strlen(time);

    if ((int) level < 0)
        level = 0;
    else if ((int) level > 3)
        level = 3;
    levelname = SG_LOGLEVEL[(int) level];
    levellen = (int) strlen(levelname);

    m.time = time;
    m.timelen = timelen;
    m.level = levelname;
    m.levellen = levellen;
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

static void
sg_dologv(sg_log_level_t level, struct sg_error *err,
          const char *msg, va_list ap)
{
    char buf[LOG_BUFSZ];
    int r, s;
#if defined _WIN32
    r = _vsnprintf_s(buf, sizeof(buf), _TRUNCATE, msg, ap);
#else
    r = vsnprintf(buf, sizeof(buf), msg, ap);
#endif
    if (r < 0)
        r = 0;
    else if ((size_t) r >= sizeof(buf))
        r = sizeof(buf) - 1;
    if (err) {
#if defined _WIN32
        if (err->code) {
            s = _snprintf_s(
                buf + r, sizeof(buf)-r, _TRUNCATE,
                ": %s (%s %ld)", err->msg, err->domain->name, err->code);
        } else {
            s = _snprintf_s(
                buf + r, sizeof(buf) - r, _TRUNCATE,
                ": %s (%s)", err->msg, err->domain->name);
        }
#else
        if (err->code) {
            s = snprintf(
                buf + r, sizeof(buf) - r,
                ": %s (%s %ld)", err->msg, err->domain->name, err->code);
        } else {
            s = snprintf(
                buf + r, sizeof(buf) - r,
                ": %s (%s)", err->msg, err->domain->name);
        }
#endif
        if (s > 0) {
            r += s;
            if ((size_t) r >= sizeof(buf))
                r = sizeof(buf) - 1;
        }
    }
    sg_dologmem(level, buf, r);
}

void
sg_logs(sg_log_level_t level, const char *msg)
{
    sg_dologmem(level, msg, strlen(msg));
}

void
sg_logf(sg_log_level_t level, const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    sg_dologv(level, NULL, msg, ap);
    va_end(ap);
}

void
sg_logv(sg_log_level_t level, const char *msg, va_list ap)
{
    sg_dologv(level, NULL, msg, ap);
}

void
sg_logerrs(sg_log_level_t level, struct sg_error *err,
           const char *msg)
{
    sg_logerrf(level, err, "%s", msg);
}

void
sg_logerrf(sg_log_level_t level, struct sg_error *err,
           const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    sg_dologv(level, err, msg, ap);
    va_end(ap);
}

void
sg_logerrv(sg_log_level_t level, struct sg_error *err,
           const char *msg, va_list ap)
{
    sg_dologv(level, err, msg, ap);
}
