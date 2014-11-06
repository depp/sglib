/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#ifndef SG_ERROR_H
#define SG_ERROR_H
#include <stdarg.h>
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file sg/error.h
 *
 * @brief Error handling.
 *
 * Functions which can signal errors often take a double pointer to
 * ::sg_error as a final argument.  This argument can either be NULL,
 * or it can point to a ::sg_error pointer.
 *
 * If the argument is NULL, then there is no place for the function to
 * store the error information, so the error information will get
 * logged instead.  If you are only going to log the error anyway, it
 * makes sense to pass NULL.
 *
 * If the argument is not NULL, then the function will report error
 * information by storing a pointer to a ::sg_error in the value
 * pointed to by the argument.  This will only happen if that pointer
 * is NULL, otherwise, a warning will be logged that an error occurred
 * and was discarded.  This means that the first error is the only
 * error that will get reported, if you reuse the same function.
 *
 * Errors can be moved and cleared using the ::sg_error_move and
 * ::sg_error_clear functions.
 *
 * By convention, most functions which signal errors have the error as
 * the last argument, and return either NULL, return a nonzero value,
 * or return a negative value to signal that an error has occurred.
 *
 * Example functions:
 *
 @code
 // Returns NULL to signal errors.
 void *func1(int x, int y, struct sg_error **err);

 // Returns nonzero to signal errors.
 int func2(char *str, struct sg_error **err);

 void explicit_error_handling(void) {
     // Must initialize to NULL.
     struct sg_error *err = NULL;

     void *ptr = func1(15, 32, &err);
     if (!ptr) {
         // Ignore the error.
         puts("Error occurred, ignoring.");
         sg_error_clear(&err);
     }

     int r = func2("Hello", &err);
     if (!ptr) {
         // Critical error, abort.
         printf("Error occurred: %s\n", (*err)->msg);
         abort();
     }
 }

 void implicit_error_handling(void) {
     void *ptr = func1(15, 32, NULL);
     // Error is automatically logged.
     if (!ptr)
         abort();
 }
 @endcode
 */

/**
 * @brief An error.
 *
 * Errors can be identified by their domain and code.  Errors can be
 * freed by calling <tt>free()</tt>, but this is not recommended, and
 * you must only do this when the reference count drops to zero.
 */
struct sg_error {
    /**
     * @brief Error reference count.
     *
     * It is recommended to not read or modify this field.
     */
    int refcount;

    /**
     * @brief The error domain.
     *
     * This allows code to identify the error.
     */
    const struct sg_error_domain *domain;

    /**
     * @brief The error message.
     *
     * This is a human-readable message.
     */
    const char *msg;

    /**
     * @brief The error code.
     *
     * The meaning of this field is domain-specific.
     */
    long code;
};

/**
 * @brief An error domain.
 */
struct sg_error_domain {
    /**
     * @brief The name of the error domain.
     */
    char name[16];
};

/**
 * @brief Set an error using a format string.
 *
 * This function should not be called directly, unless you are
 * creating your own error domain.
 *
 * @param err The error to set, may be NULL.
 * @param dom The error domain.
 * @param code The error code.
 * @param fmt The error message, a format string.
 * @param ... The format arguments.
 */
void
sg_error_setf(struct sg_error **err, const struct sg_error_domain *dom,
              long code, const char *fmt, ...);

/**
 * @brief Set an error using a format string and argument list.
 *
 * This function should not be called directly, unless you are
 * creating your own error domain.
 *
 * @param err The error to set, may be NULL.
 * @param dom The error domain.
 * @param code The error code.
 * @param fmt The error message, a format string.
 * @param ap The argument list.
 */
void
sg_error_setv(struct sg_error **err, const struct sg_error_domain *dom,
              long code, const char *fmt, va_list ap);

/**
 * @brief Set an error using a string.
 *
 * This function should not be called directly, unless you are
 * creating your own error domain.
 *
 * @param err The error to set, may be NULL.
 * @param dom The error domain.
 * @param code The error code.
 * @param msg The error message, this will be copied verbatim.
 */
void
sg_error_sets(struct sg_error **err, const struct sg_error_domain *dom,
              long code, const char *msg);

/**
 * @brief Move an error from one location to another.
 *
 * This will give warnings for discarded errors as expected.
 *
 * @param dest The destination error location, can be NULL.
 * @param src The source error location.
 */
void
sg_error_move(struct sg_error **dest, struct sg_error **src);

/**
 * @brief Clear an error location.
 *
 * @param err The error to clear.
 */
void
sg_error_clear(struct sg_error **err);

/**
 * @brief Error domain for out of memory errors.
 */
extern const struct sg_error_domain SG_ERROR_NOMEM;

/**
 * @brief Create an error indicating an out of memory condition.
 *
 * @param err The error to initialize, can be NULL.
 */
void
sg_error_nomem(struct sg_error **err);

/**
 * @brief Error domain for canceled operations.
 */
extern const struct sg_error_domain SG_ERROR_CANCEL;

/**
 * @brief Error domain for invalid function arguments.
 */
extern const struct sg_error_domain SG_ERROR_INVALID;

/**
 * @brief Create an error indicating an invalid argument.
 *
 * @param err The error to initialize, can be NULL.
 * @param function The function name.
 * @param argument The argument name.
 */
void
sg_error_invalid(struct sg_error **err,
                 const char *function, const char *argument);

/**
 * @brief Error domain for files not found.
 */
extern const struct sg_error_domain SG_ERROR_NOTFOUND;

/**
 * @brief Create an error indicating that a file was not found.
 *
 * @param err The error to initialize, can be NULL.
 * @param path The path to the file which was not found.
 */
void
sg_error_notfound(struct sg_error **err, const char *path);

/**
 * @brief Error domain for an invalid pathname.
 */
extern const struct sg_error_domain SG_ERROR_INVALPATH;

/**
 * @brief Error domain for corrupted data files.
 */
extern const struct sg_error_domain SG_ERROR_DATA;

/**
 * @brief Create an error for a corrupt data file.
 *
 * @param err The error to initialize, can bu NULL.
 * @param fmtname The name of the data format.
 */
void
sg_error_data(struct sg_error **err, const char *fmtname);

#if defined(_WIN32) || defined(DOXYGEN)

/**
 * @brief Domain for Windows system errors.
 */
extern const struct sg_error_domain SG_ERROR_WINDOWS;

/**
 * @brief Create an error from a Windows error code.
 *
 * @param err The error to initialize, can be NULL.
 * @param code The Windows error code.
 */
void
sg_error_win32(struct sg_error **err, unsigned long code);

/**
 * @brief Create an error from an <tt>HRESULT</tt>.
 *
 * @param err The error to initialize, can be NULL.
 * @param code The HRESULT code.
 */
void
sg_error_hresult(struct sg_error **err, long code);

#endif

#if !defined(_WIN32) || defined(DOXYGEN)

/**
 * @brief Domain for POSIX error codes.
 */
extern const struct sg_error_domain SG_ERROR_ERRNO;

/**
 * @brief Create an error from a POSIX error code.
 *
 * @param err The error to initialize, can be null.
 * @param code The error code.
 */
void
sg_error_errno(struct sg_error **err, int code);

#endif

/**
 * @brief Domain for errors from getaddrinfo.
 */
extern const struct sg_error_domain SG_ERROR_GETADDRINFO;

/**
 * @brief Create a getaddrinfo domain error.
 *
 * @param err The error to initialize, can be null.
 * @param code The getaddrinfo error code.
 */
void
sg_error_gai(struct sg_error **err, int code);

/**
 * @brief Domain for errors for disabled features.
 */
extern const struct sg_error_domain SG_ERROR_DISABLED;

/**
 * @brief Create a disabled feature error.
 *
 * @param err The error to initialize, can be null.
 * @param name The name of the disabled feature.
 */
void
sg_error_disabled(struct sg_error **err, const char *name);

#ifdef __cplusplus
}
#endif
#endif
