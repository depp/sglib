/* Copyright 2012 Dietrich Epp <depp@zdome.net> */
#ifndef SG_AIO_H
#define SG_AIO_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
struct sg_buffer;
struct sg_error;
struct sg_aio_request;

/**
 * @file sg/aio.h
 *
 * @brief Asynchronous IO.
 *
 * Asynchronous IO is a bit tricky to get right.  This module only
 * provides support for reading an entire file with asynchronous IO or
 * canceling a pending asynchronous IO operation, no other operations
 * are supported.
 *
 * An asynchronous IO request has a duration that starts once the
 * request is made and ends once the user-supplied callback function
 * returns.  If errors are encountered or if the request is canceled,
 * the callback function is called.  The callback function is
 * generally responsible for ensuring that no thread attempts to
 * cancel the request after the callback returns.
 *
 * The callback function may be called from an arbitrary thread, and
 * should do a relatively minimal amount of work before returning.
 * Small files with simple formats may be decoded in the callback, but
 * the processing of sophisticated or large files (images, audio,
 * video) should be offloaded to another thread.
 */

/**
 * @brief Initialize the asynchronous IO subsystem.
 */
void
sg_aio_init(void);

/**
 * @brief Error that indicates that the AIO operation was canceled.
 */
extern const struct sg_error_domain SG_AIO_CANCEL;

/**
 * @brief Callback for completed AIO requests.
 *
 * @param cxt User-supplied callback parameter
 *
 * @param buf If the operation was successful, then a pointer to a
 * buffer containing the file data.  Otherwise, @c NULL.  The buffer
 * should not be modified by the callback.
 *
 * @param err If the operation was successful, then @c NULL.
 * Otherwise, a pointer to an error explaining why the operation
 * failed.  The error should not be modified by the callback.
 */
typedef void
(*sg_aio_callback_t)(void *cxt,
                     struct sg_buffer *buf,
                     struct sg_error *err);

/**
 * @brief Request a file to be loaded asynchronously.
 *
 * @param path File path, relative to the game root directory.  Does
 * not need to be NUL-terminated, sanitized, or normalized.
 *
 * @param pathlen Length of the file path.
 *
 * @param flags File mode flags.  ::SG_RDONLY is assumed.
 *
 * @param extensions A NUL-terminated list of extensions to try,
 * separated by @c ':'.  The extensions should not start with @c '.'.
 * For example, @c "png:jpg".
 *
 * @param maxsize Maximum file size.  If the file is larger than this,
 * the operation will fail with the ::SG_ERROR_DATA domain.
 *
 * @param cxt User parameter for the callback function.
 *
 * @param callback Callback function.
 *
 * @param err On failure, the error.
 *
 * @return On success, a pointer to an asynchronous IO request object.
 * The object is valid until the callback function returns; the
 * callback function should make sure that the object is not used
 * afterwards.
 */
struct sg_aio_request *
sg_aio_request(const char *path, size_t pathlen, int flags,
               const char *extensions, size_t maxsize,
               void *cxt, sg_aio_callback_t callback,
               struct sg_error **err);

/**
 * @brief Attempt to cancel a pending asynchronous IO request.
 *
 * This will not necessarily cancel the request; the request may be in
 * a state where cancellation is impossible.
 *
 * If the request is cancelled, then the callback will eventually be
 * called with a @c NULL buffer and an error in the ::SG_AIO_CANCEL
 * domain.
 *
 * @param ioreq The IO request.
 */
void
sg_aio_cancel(struct sg_aio_request *ioreq);

#ifdef __cplusplus
}
#endif
#endif
