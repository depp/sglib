#ifndef IMPL_ERROR_H
#define IMPL_ERROR_H
#include <stdarg.h>
#ifdef __cplusplus
extern "C" {
#endif

struct error {
    const struct error_domain *domain;
    const char *msg;
    long code;
};

struct error_domain {
    char name[16];
};

/* Setting errors: When an error is set, it is expected that the
   destination (err) is a pointer to a NULL pointer (*err == NULL).  A
   new error object will be created on the heap and assigned to *err.
   If *err is not NULL, then the new error is discarded with a warning
   message (we want to report the earliest error, usually).  If err is
   NULL, then the error is discarded with a warning.  */

/* Set error using a formatted message.  */
void
error_set(struct error **err, const struct error_domain *dom,
          long code, const char *fmt, ...);

/* Set error using a formatted message and a va_list.  */
void
error_setv(struct error **err, const struct error_domain *dom,
           long code, const char *fmt, va_list ap);

/* Set error using literal sting.  */
void
error_sets(struct error **err, const struct error_domain *dom,
           long code, const char *msg);

/* Moving errors: When an error is moved, it is as if the destination
   error variable is set to a copy of the source error, and the source
   error is cleared.  Use this instead of assignment.  */
void
error_move(struct error **dest, struct error **src);

/* Clear an error field.  */
void
error_clear(struct error **err);

/* ===== Error Domains ===== */

/* A special domain for "file not found".  */
extern const struct error_domain ERROR_NOTFOUND;

/* A special domain for "out of memory".  */
extern const struct error_domain ERROR_NOMEM;

void
error_set_notfound(struct error **err, const char *path);

void
error_set_nomem(struct error **err);

#if defined(_WIN32)
/* Put Windows-specific error domains here */
#endif

const struct error_domain ERROR_ERRNO;

void
error_errno(struct error **err, int code);

#ifdef __cplusplus
}
#endif
#endif
