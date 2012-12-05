/* Copyright 2012 Dietrich Epp <depp@zdome.net> */
#ifndef SG_ERROR_H
#define SG_ERROR_H
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
sg_error_setf(struct sg_error **err, const struct sg_error_domain *dom,
              long code, const char *fmt, ...);

/* Set error using a formatted message and a va_list.  */
void
sg_error_setv(struct sg_error **err, const struct sg_error_domain *dom,
              long code, const char *fmt, va_list ap);

/* Set error using a string as the message.  The string may be
   NULL.  */
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

extern const struct sg_error_domain SG_ERROR_WINDOWS;

void
sg_error_win32(struct sg_error **err, unsigned long code);

void
sg_error_hresult(struct sg_error **err, long code);

#else

extern const struct sg_error_domain SG_ERROR_ERRNO;

void
sg_error_errno(struct sg_error **err, int code);

#endif

extern const struct sg_error_domain SG_ERROR_GETADDRINFO;

void
sg_error_gai(struct sg_error **err, int code);

#ifdef __cplusplus
}
#endif
#endif
