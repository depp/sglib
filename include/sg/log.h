/* Copyright 2012-2013 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#ifndef SG_LOG_H
#define SG_LOG_H
#include "sg/defs.h"
#include <stdarg.h>
#ifdef __cplusplus
extern "C" {
#endif
struct sg_error;
/**
 * @file sg/log.h
 *
 * @brief Logging.
 */

/**
 * @brief Log levels.
 *
 * There is no "fatal" level.  Fatal errors should be reported with
 * sg_sys_abort() or similar functions instead.
 */
typedef enum {
    /**
     * @brief Fine-grained information, not always useful.
     */
    SG_LOG_DEBUG,

    /**
     * @brief Miscellaneous normal runtime events.
     */
    SG_LOG_INFO,

    /**
     * @brief Potentially harmful situations, possible unexpected
     * behavior.
     */
    SG_LOG_WARN,

    /**
     * @brief Runtime errors.
     */
    SG_LOG_ERROR
} sg_log_level_t;

/**
 * @brief Log a string message.
 *
 * @param level The log level for the message.
 * @param msg The message, a NUL-terminated UTF-8 string.
 */
void
sg_logs(sg_log_level_t level, const char *msg);

/**
 * @brief Log a formatted string message.
 *
 * @param level The log level for the message.
 * @param msg The message format string, NUL-terminated UTF-8.
 * @param ... The format parameters.
 */
SG_ATTR_FORMAT(printf, 2, 3)
void
sg_logf(sg_log_level_t level, const char *msg, ...);

/**
 * @brief Log a formatted string message.
 *
 * @param level The log level for the message.
 * @param msg The message format string, NUL-terminated UTF-8.
 * @param ap The format parameters.
 */
void
sg_logv(sg_log_level_t level, const char *msg, va_list ap);

/**
 * @brief Log a string message and an error.
 *
 * @param level The log level for the message.
 * @param msg The message, a NUL-terminated UTF-8 string.
 */
void
sg_logerrs(sg_log_level_t level, struct sg_error *err,
           const char *msg);

/**
 * @brief Log a formatted string message and an error.
 *
 * @param level The log level for the message.
 * @param err The error to log.
 * @param msg The message format string, NUL-terminated UTF-8.
 * @param ... The format parameters.
 */
SG_ATTR_FORMAT(printf, 3, 4)
void
sg_logerrf(sg_log_level_t level, struct sg_error *err,
           const char *msg, ...);

/**
 * @brief Log a formatted string message and an error.
 *
 * @param level The log level for the message.
 * @param err The error to log.
 * @param msg The message format string, NUL-terminated UTF-8.
 * @param ap The format parameters.
 */
void
sg_logerrv(sg_log_level_t level, struct sg_error *err,
           const char *msg, va_list ap);

#ifdef __cplusplus
}
#endif
#endif
