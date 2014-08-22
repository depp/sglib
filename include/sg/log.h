/* Copyright 2012-2013 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#ifndef SG_LOG_H
#define SG_LOG_H
#include "sg/attribute.h"
#include <stdarg.h>
#ifdef __cplusplus
extern "C" {
#endif
struct sg_error;
/**
 * @file sg/log.h
 *
 * @brief Logging.
 *
 * The logging API is modeled after popular logging frameworks,
 * although somewhat simplified.
 *
 * Each logger other than the root logger is identified by a name.  A
 * logger is logger is an "ancestor" of another logger if the
 * ancestor's name followed by a dot is a prefix of the descendent's
 * name.  The root logger is also the ancestor of all other loggers.
 *
 * Each logger and message has a log level.  If the logger's log level
 * is not set explicitly, then it is inherited from its closest
 * ancestor.  The root logger always has a log level set.  Messages
 * are only logged when the message's log level is equal to the
 * logger's log level or higher.
 */

/**
 * @brief Log levels.
 *
 * There is no "fatal" level.  Any logger expected to respond to a
 * fatal error should respond to a non-fatal error as well, and fatal
 * error should generally be reported with sg_sys_abort() or similar
 * functions.
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
 * @brief A logger.
 *
 * There are other fields not visible in this structure.  Loggers
 * should not be created directly or copied.  Only sg_logger_get()
 * should be used to get a logger.
 */
struct sg_logger {
    /**
     * @brief The computed log level, do not modify.
     */
    sg_log_level_t level;
};

/**
 * @brief Get the logger with the given name.
 *
 * The result should not be freed and will exist until the program
 * exits.
 *
 * @param name The name of the logger to get.  If this is NULL or
 * empty, this will specify the root logger.
 * @return The logger with the given name, which is created if
 * necessary.
 */
struct sg_logger *
sg_logger_get(const char *name);

/**
 * @brief Log a string message.
 *
 * @param logger The logger.
 * @param level The log level for the message.
 * @param msg The message, a NUL-terminated UTF-8 string.
 */
void
sg_logs(struct sg_logger *logger, sg_log_level_t level,
        const char *msg);

/**
 * @brief Log a formatted string message.
 *
 * @param logger The logger.
 * @param level The log level for the message.
 * @param msg The message format string, NUL-terminated UTF-8.
 * @param ... The format parameters.
 */
SG_ATTR_FORMAT(printf, 3, 4)
void
sg_logf(struct sg_logger *logger, sg_log_level_t level,
        const char *msg, ...);

/**
 * @brief Log a formatted string message.
 *
 * @param logger The logger.
 * @param level The log level for the message.
 * @param msg The message format string, NUL-terminated UTF-8.
 * @param ap The format parameters.
 */
void
sg_logv(struct sg_logger *logger, sg_log_level_t level,
        const char *msg, va_list ap);

/**
 * @brief Log a string message and an error.
 *
 * @param logger The logger.
 * @param level The log level for the message.
 * @param msg The message, a NUL-terminated UTF-8 string.
 */
void
sg_logerrs(struct sg_logger *logger, sg_log_level_t level,
           struct sg_error *err, const char *msg);

/**
 * @brief Log a formatted string message and an error.
 *
 * @param logger The logger.
 * @param level The log level for the message.
 * @param err The error to log.
 * @param msg The message format string, NUL-terminated UTF-8.
 * @param ... The format parameters.
 */
SG_ATTR_FORMAT(printf, 4, 5)
void
sg_logerrf(struct sg_logger *logger, sg_log_level_t level,
           struct sg_error *err, const char *msg, ...);

/**
 * @brief Log a formatted string message and an error.
 *
 * @param logger The logger.
 * @param level The log level for the message.
 * @param err The error to log.
 * @param msg The message format string, NUL-terminated UTF-8.
 * @param ap The format parameters.
 */
void
sg_logerrv(struct sg_logger *logger, sg_log_level_t level,
           struct sg_error *err, const char *msg, va_list ap);

#ifdef __cplusplus
}
#endif
#endif
