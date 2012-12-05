/* Copyright 2012 Dietrich Epp <depp@zdome.net> */
#ifndef SG_LOG_IMPL_H
#define SG_LOG_IMPL_H
#include "sg/log.h"
#include <stddef.h>

struct sg_log_msg {
    const char *date;
    size_t datelen;
    const char *level;
    size_t levellen;
    const char *name;
    size_t namelen;
    const char *msg;
    size_t msglen;
    sg_log_level_t levelval;
};

struct sg_log_listener {
    void (*msg)(struct sg_log_listener *, struct sg_log_msg *);
    void (*destroy)(struct sg_log_listener *);
};

void
sg_log_listen(struct sg_log_listener *listener);

void
sg_log_console_init(void);

void
sg_log_network_init(void);

#endif
