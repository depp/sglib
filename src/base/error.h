#ifndef IMPL_ERROR_H
#define IMPL_ERROR_H
#include <stdarg.h>
#ifdef __cplusplus
extern "C" {
#endif

struct sg_error {
    int refcount;
    const struct sg_error_domain *domain;
    const char *msg;
    long code;
};

struct sg_error_domain {
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
sg_error_set(struct sg_error **err, const struct sg_error_domain *dom,
             long code, const char *fmt, ...);

/* Set error using a formatted message and a va_list.  */
void
sg_error_setv(struct sg_error **err, const struct sg_error_domain *dom,
              long code, const char *fmt, va_list ap);

/* Set error using literal sting.  */
void
sg_error_sets(struct sg_error **err, const struct sg_error_domain *dom,
              long code, const char *msg);

/* Moving errors: When an error is moved, it is as if the destination
   error variable is set to a copy of the source error, and the source
   error is cleared.  Use this instead of assignment.  */
void
sg_error_move(struct sg_error **dest, struct sg_error **src);

void
sg_error_clear(struct sg_error **err);

/* ===== Error Domains ===== */

/* A domain for "invalid pathname".  */
extern const struct sg_error_domain SG_ERROR_INVALPATH;

/* A domain for "file not found".  */
extern const struct sg_error_domain SG_ERROR_NOTFOUND;

/* A domain for "out of memory".  */
extern const struct sg_error_domain SG_ERROR_NOMEM;

/* A domain for errors in data file formats.  */
extern const struct sg_error_domain SG_ERROR_DATA;

/* Produce a "file not found" error for the given path.  */
void
sg_error_notfound(struct sg_error **err, const char *path);

/* Produce a generic "out of memory" error.  */
void
sg_error_nomem(struct sg_error **err);

/* Produce a "corrupt file" error for a file with the given format:
   PNG, JPEG, etc.  */
void
sg_error_data(struct sg_error **err, const char *fmtname);

#if defined(_WIN32)
/* Put Windows-specific error domains here */
#endif

extern const struct sg_error_domain SG_ERROR_ERRNO;

void
sg_error_errno(struct sg_error **err, int code);

#ifdef __cplusplus
}
#endif
#endif