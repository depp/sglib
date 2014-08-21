/* Copyright 2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "sg/error.h"
#include "sg/log.h"
#include <stddef.h>
#include <stdio.h>

struct sg_logger sg_logger = { SG_LOG_DEBUG };

struct sg_logger *
sg_logger_get(const char *name)
{
    (void) name;
    return &sg_logger;
}

static const char SG_LOGLEVEL[4][6] = {
    "DEBUG", "INFO", "WARN", "ERROR"
};

void
sg_logs(struct sg_logger *logger, sg_log_level_t level,
        const char *msg)
{
    sg_logf(logger, level, "%s", msg);
}

void
sg_logf(struct sg_logger *logger, sg_log_level_t level,
        const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    sg_logerrv(logger, level, NULL, msg, ap);
    va_end(ap);
}

void
sg_logv(struct sg_logger *logger, sg_log_level_t level,
        const char *msg, va_list ap)
{
    sg_logerrv(logger, level, NULL, msg, ap);
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
    va_start(ap, msg);
    sg_logerrv(logger, level, err, msg, ap);
}

void
sg_logerrv(struct sg_logger *logger, sg_log_level_t level,
           struct sg_error *err, const char *msg, va_list ap)
{
    (void) logger;
    fprintf(stderr, "%s: ", SG_LOGLEVEL[level]);
    vfprintf(stderr, msg, ap);
    if (err != NULL) {
        fprintf(
            stderr, " (domain: %s, msg: %s, code: %ld)",
            err->domain->name, err->msg, err->code);
    }
    fputc('\n', stderr);
}
