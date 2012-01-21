#include "clock.h"
#include "cvar.h"
#include "log.h"
#include "log_impl.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOG_BUFSZ 256
#define MAX_LISTENERS 4
#define LOG_INHERIT ((sg_log_level_t) -1)

struct sg_logger_obj {
    struct sg_logger head;
    sg_log_level_t level;
    char *name;
    size_t namelen;
    struct sg_logger_obj *child, *next;
};

static struct sg_logger_obj sg_logger_root;
static struct sg_log_listener *sg_listeners[4];

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
    sg_logs(&sg_logger_root.head, LOG_WARN,
            "Too many log listeners, log listener dropped.");
}

static sg_log_level_t
sg_logger_conflevel(const char *name, size_t len)
{
    const char *v;
    char buf[128];
    int r;
    if (!len) {
        name = "root";
        len = 4;
    }
    if (len > sizeof(buf) - 1)
        return LOG_INHERIT;
    memcpy(buf, name, len);
    buf[len] = '\0';
    r = sg_cvar_gets("log.level", buf, &v);
    if (r) {
        if (!strcmp(v, "debug"))
            return LOG_DEBUG;
        if (!strcmp(v, "info"))
            return LOG_INFO;
        if (!strcmp(v, "warn"))
            return LOG_WARN;
        if (!strcmp(v, "error"))
            return LOG_ERROR;
    }
    return LOG_INHERIT;
}

void
sg_log_init(void)
{
    sg_log_level_t level = sg_logger_conflevel(NULL, 0);
    sg_logger_root.head.level = level == LOG_INHERIT ? LOG_WARN : level;
    sg_logger_root.level = level;
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
sg_logger_new(struct sg_logger_obj *parent,
              const char *name, size_t namelen)
{
    struct sg_logger_obj *np;
    sg_log_level_t level;
    np = malloc(sizeof(*np) + namelen);
    if (!np)
        abort();
    level = sg_logger_conflevel(name, namelen);
    np->head.level = level == LOG_INHERIT ? parent->head.level : level;
    np->level = level;
    np->name = (char *) (np + 1);
    np->namelen = namelen;
    np->child = NULL;
    np->next = parent->child;
    parent->child = np;
    memcpy(np->name, name, namelen);
    return np;
}

struct sg_logger *
sg_logger_get(const char *name)
{
    size_t len, clen, off = 0;
    char *q;
    struct sg_logger_obj *lp = &sg_logger_root, *np;
    if (!name || !*name)
        return &lp->head;
    len = strlen(name);
    for (off = 0; off < len; ++off) {
        if (!((name[off] >= 'a' && name[off] <= 'z') ||
              (name[off] >= 'A' && name[off] <= 'Z') ||
              (name[off] >= '0' && name[off] <= '9') ||
              name[off] == '-' || name[off] == '.' || name[off] == '_'))
            abort();
    }
    while (1) {
        q = memchr(name + off, len - off, '.');
        clen = q ? (size_t) (q - (name + off )) : (len - off);
        for (np = lp->child; np; np = np->next)
            if (!memcmp(np->name + off, name + off, clen))
                break;
        if (!np)
            np = sg_logger_new(lp, name, off + clen);
        off += clen;
        lp = np;
        if (!name[off])
            return &lp->head;
        if (name[off] == '.') {
            off += 1;
            if (!name[off])
                abort();
        }
    }
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

    datelen = sg_clock_getdate(date);
    if ((int) level < 0)
        level = 0;
    else if ((int) level > 3)
        level = 3;
    levelname = SG_LOGLEVEL[(int) level];
    levellen = strlen(levelname);

    m.date = date;
    m.datelen = datelen;
    m.level = levelname;
    m.levellen = levellen;
    m.name = lp->name;
    m.namelen = lp->namelen;
    m.msg = msg;
    m.msglen = len;
    m.levelval = level;

    for (i = 0; i < MAX_LISTENERS; ++i) {
        p = sg_listeners[i];
        if (p) {
            sg_listeners[i] = NULL;
            p->msg(p, &m);
            sg_listeners[i] = p;
        }
    }
}

static void
sg_dologv(struct sg_logger *logger, sg_log_level_t level,
          const char *msg, va_list ap)
{
    char buf[LOG_BUFSZ];
    int r;
#ifdef _MSC_VER
    r = _vsnprintf(buf, sizeof(buf), msg, ap);
#else
    r = vsnprintf(buf, sizeof(buf), msg, ap);
#endif
    if (r < 0 || (size_t) r > sizeof(buf))
        r = sizeof(buf) - 1;
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
    sg_dologv(logger, level, msg, ap);
    va_end(ap);
}

void
sg_logv(struct sg_logger *logger, sg_log_level_t level,
        const char *msg, va_list ap)
{
    if (level < logger->level)
        return;
    sg_dologv(logger, level, msg, ap);
}
