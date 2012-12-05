/* The logging API is modeled after the popular log4j, except it is
   simplified and modified to work with the existing configuration
   system.

   Each logger other than the root logger is identified by a name.  A
   logger is an "ancestor" of another logger if the ancestor's name
   followed by a dot is a prefix of the descendent's name.  Each
   logger also has a log level.  If the log level is not set
   explicitly for a logger, then it is inherited from its closest
   ancestor.  The root logger always has a log level set.

   When a log message is sent to a logger, the message is only
   emmitted if the level of the message is equal to or greater than
   the level of the logger.  */
#ifndef BASE_LOG_H
#define BASE_LOG_H
#include <stdarg.h>
#ifdef __cplusplus
extern "C" {
#endif

struct sg_error;

typedef enum {
    LOG_DEBUG, /* Fine-graned information */
    LOG_INFO,  /* Runtime events, e.g., application progress */
    LOG_WARN,  /* Potentially harmful situations */
    LOG_ERROR  /* Runtime errors */

    /* There is no "FATAL" level.  Any logger expected to respond to a
       fatal error should respond to a non-fatal erorr as well.  */
} sg_log_level_t;


/* A logger.  There are plenty of extra fields.  Never create a logger
   directly or copy a logger, instead use sg_logger_get.  */
struct sg_logger {
    /* The computed log level, do not modify.  */
    sg_log_level_t level;
};

/* Initialize logging subsystem.  */
void
sg_log_init(void);

/* Shut down logging system: close sockets, etc.  */
void
sg_log_term(void);

/* Get a logger with the given name.  The result should not be freed.
   If the name is NULL or empty, the root logger is returned.  */
struct sg_logger *
sg_logger_get(const char *name);

/* Emit a string message.  */
void
sg_logs(struct sg_logger *logger, sg_log_level_t level,
        const char *msg);

/* Emit a format string message.  */
void
sg_logf(struct sg_logger *logger, sg_log_level_t level,
        const char *msg, ...);

/* Emit a format string message.  */
void
sg_logv(struct sg_logger *logger, sg_log_level_t level,
        const char *msg, va_list ap);

/* Same as above, but with an error object to show.  */

void
sg_logerrs(struct sg_logger *logger, sg_log_level_t level,
           struct sg_error *err, const char *msg);


void
sg_logerrf(struct sg_logger *logger, sg_log_level_t level,
           struct sg_error *err, const char *msg, ...);

void
sg_logerrv(struct sg_logger *logger, sg_log_level_t level,
           struct sg_error *err, const char *msg, va_list ap);

#ifdef __cplusplus
}
#endif
#endif
