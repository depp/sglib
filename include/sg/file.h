/* Copyright 2012-2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#ifndef SG_FILE_H
#define SG_FILE_H
#include "sg/atomic.h"
#include "sg/attribute.h"
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
struct sg_error;

/**
 * @file sg/file.h
 *
 * @brief Low-level file interface.
 *
 * This interface abstracts away differences in how files are opened
 * across platforms, sanitizes file paths, and manages file search
 * paths.  All files opened through this interface must exist below
 * the root of one of the search paths, file paths are sanitized to
 * ensure this.
 */

/**
 * @brief Flags for opening files.
 */
enum {
    /** @brief Open the file for reading.  */
    SG_RDONLY = 00,
    /** @brief Open the file for writing.  */
    SG_WRONLY = 01,
    /** @brief Only search the user data path.  */
    SG_USERONLY = 02,
    /** @brief Only search the application data path.  */
    SG_DATAONLY = 04
};

enum {
    /** @brief Maximum length of a path, including NUL byte.  */
    SG_MAX_PATH = 128
};

/**
 * @brief Normalize and sanitize a path.
 *
 * The resulting path will use '/' as a path separator, will not start
 * with a '/', and each component will be a non-empty string
 * containing only alphanumeric characters, '-', '_', and '.'.  No
 * component will start or end with a period, and now component will
 * be a reserved device name on Windows.  If the input path refers to
 * a directory, the output will either end with '/' or be empty.
 *
 * @param buf The buffer to store the result, with length SG_MAX_PATH.
 * @param path The path to normalize.
 * @param pathlen The length of the path to normalize, in bytes.
 * @param err On failure, the error.
 * @return On success, a positive number or zero, indicating the
 * length of the result.  On failure, a negative number.
 */
int
sg_path_norm(char *buf, const char *path, size_t pathlen,
             struct sg_error **err);

/**
 * @brief Join a relative path to a base path.
 *
 * The relative path is normalized and sanitized.  It is assumed that
 * the base path is already normalized and sanitized.  This resolves
 * paths in the same way that relative paths are resolved with URIs.
 * Examples:
 *
 * - base: `dir/file.txt`, path: `image.jpg`, result: `dir/image.jpg`
 * - base: `dir/subdir/`, path: `..`, result: `dir/`
 *
 * @param buf The base path and result, with size SG_MAX_PATH.
 * @param path The relative path to append.
 * @param pathlen The length of the relative path, in bytes.
 * @param err On failure, the error.
 * @return On success, a positive number or zero, indicating the
 * length of the result.  On failure, a negative number.
 */
int
sg_path_join(char *buf, const char *path, size_t pathlen,
             struct sg_error **err);

/**
 * @brief A buffer of data read from a file.
 *
 * The buffer is reference counted.
 */
struct sg_buffer {
    /**
     * @private @brief Reference count, do not modify.
     */
    sg_atomic_t refcount;

    /**
     * @brief Pointer to buffer data.  This buffer also contains one
     * zero byte after the end of the buffer data.
     */
    void *data;

    /**
     * @brief Length of buffer data, not counting the extra zero at
     * the end.
     */
    size_t length;
};

/**
 * @brief Increment a buffer's reference count.
 */
void
sg_buffer_incref(struct sg_buffer *fbuf);

/**
 * @brief Decrement a buffer's reference count.
 */
void
sg_buffer_decref(struct sg_buffer *fbuf);

/**
 * @brief An abstract file.
 *
 * Do not copy this structure.
 */
struct sg_file {
    /**
     * @brief Read data.
     *
     * @param fp This file.
     * @param buf The destination buffer.
     * @param amt The number of bytes to read.
     * @return The number of bytes read, 0 if no bytes were read
     * because the end of the file has been reached, or a negative
     * number if an error occurred.
     */
    int (*read)(struct sg_file *fp, void *buf, size_t amt);

    /**
     * @brief Write data.
     *
     * @param fp This file.
     * @param buf The source buffer.
     * @param amt The number of bytes to write.
     * @return The number of bytes written, or a negative number if an
     * error occurred.
     */
    int (*write)(struct sg_file *fp, const void *buf, size_t amt);

    /**
     * @brief Commit changes to the file.
     *
     * This is only valid for files which are open for writing.  After
     * this function is called, only `close` should be called.
     *
     * @param fp This file.
     * @return Zero if successful, non-zero if an error occurred.
     */
    int (*commit)(struct sg_file *fp);

    /**
     * @brief Close the file handle and free this structure.
     *
     * @param fp This file.
     */
    void (*close)(struct sg_file *fp);

    /**
     * @brief Get the file's length.
     *
     * @param fp This file.
     * @return The file's length, or a negative number if an error
     * occurred.
     */
    int64_t (*length)(struct sg_file *fp);

    /**
     * @brief Seek to a new file position.
     *
     * @param fp This file.
     * @param off The target offset.
     * @param whence The interpretation of the file offset.
     */
    int64_t (*seek)(struct sg_file *fp, int64_t off, int whence);

    /**
     * @brief The most recent error for operations on this file.
     */
    struct sg_error *err;
};

/**
 * @brief Open a file.
 *
 * The path need not be NUL-terminated, sanitized, or normalized.
 *
 * When opening a file for reading, this will search through the user
 * and application search paths for a file with the given relative
 * path.  If a list of extensions is specified, then each of the
 * extensions, from a colon-delimited list, will be tried at each
 * point in the path.  Empty extensions are ignored.
 *
 * For example, with search paths `"a"` and `"b"`, path `"file"`, and
 * extensions `"png:jpg"`, the following paths are tried in order:
 *
 * - `a/file.png`
 * - `a/file.jpg`
 * - `b/file.png`
 * - `b/file.jpg`
 *
 * When opening a file for writing, the file is created in the user
 * data path, and the list of extensions should be NULL.
 *
 * @param path The path to the file, relative to the search paths.
 * @param pathlen The length of the path, in bytes.
 * @param flags How to open the file.
 * @param extensions List of extensions or NULL.
 * @param err On failure, the error.
 * @return A file handle, or NULL if an error occurred.
 */
struct sg_file *
sg_file_open(const char *path, size_t pathlen, int flags,
             const char *extensions, struct sg_error **err);

/**
 * @brief Get the remaining contents of a file starting from the
 * current position.
 *
 * If the amount of data remaining is larger than the specified
 * maximum size, the operation will fail with an error in the
 * ::SG_ERROR_DATA domain, but the file position will have changed.
 * Errors will be recorded in the file handle structure.
 *
 * @param fp The file.
 * @param maxsize Maximum amount of data to read.
 * @return A data buffer, or NULL if an error occurred.
 */
struct sg_buffer *
sg_file_readall(struct sg_file *fp, size_t maxsize);

/**
 * @brief Get the contents of a file given its path.
 *
 * If the file is larger than the specified maximum size,
 * the operation will fail with the ::SG_ERROR_DATA domain.
 *
 * See sg_file_open() for a more detailed description of the
 * parameters.
 *
 * @param path  The path to the file, relative to the seacrh paths.
 * @param pathlen The length of the file path, in bytes.
 * @param flags How to open the file.  ::SG_RDONLY is implied.
 * @param extensions List of extensions or NULL.
 * @param maxsize Maximum file size.
 * @param err On failure, the error.
 * @return A data buffer, or NULL if an error occurred.
 */
struct sg_buffer *
sg_file_get(const char *path, size_t pathlen, int flags,
            const char *extensions,
            size_t maxsize, struct sg_error **err);

#ifdef __cplusplus
}
#endif
#endif
