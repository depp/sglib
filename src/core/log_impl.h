/* Copyright 2012-2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "sg/log.h"
#include <stddef.h>

struct sg_log_msg {
    const char *time;
    size_t timelen;
    const char *level;
    size_t levellen;
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
