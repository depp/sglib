#include "clock.h"
#include "log.h"
#include "log_impl.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOG_BUFSZ 256
#define MAX_LISTENERS 4

struct sg_logger_obj {
    struct sg_logger head;
    sg_log_level_t level;
    sg_log_level_t ilevel;
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

void
sg_log_init(void)
{
    sg_logger_root.head.level = LOG_WARN;
    sg_logger_root.level = LOG_WARN;
    sg_log_console_init();
}

void
sg_log_term(void)
{
    int i;
    for (i = 0; i < MAX_LISTENERS; ++i)
        if (sg_listeners[i])
            sg_listeners[i]->destroy(sg_listeners[i]);
}

static sg_log_level_t
sg_logger_conflevel(const char *name, size_t namelen)
{
    (void) name;
    (void) namelen;
    return LOG_INHERIT;
}

struct sg_logger_obj *
sg_logger_new(struct sg_logger_obj *parent,
              const char *name, size_t namelen)
{
    struct sg_logger_obj *np;
    np = malloc(sizeof(*np) + namelen);
    if (!np)
        abort();
    np->level = sg_logger_conflevel(name, namelen);
    np->ilevel = parent->level;
    np->head.level = np->level == LOG_INHERIT ? np->ilevel : np->level;
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
    size_t len = strlen(name), clen, off = 0;
    char *q;
    struct sg_logger_obj *lp = &sg_logger_root, *np;
    if (!len)
        return &lp->head;
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
    struct sg_logger_obj *lp = (struct sg_logger_obj *) logger;
    struct sg_log_msg m;
    const char *levelname;
    char date[SG_DATE_LEN];
    int datelen, levellen;

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
