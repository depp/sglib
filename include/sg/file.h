/* Copyright 2012-2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#ifndef SG_FILE_H
#define SG_FILE_H
#include "sg/atomic.h"
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
struct sg_error;

/**
 * @file sg/file.h
 *
 * @brief Common file interface.
 *
 * This interface abstracts away differences in how files are opened
 * across platforms, sanitizes file paths, and manages file search
 * paths.
 */

enum {
    /** @brief Maximum length of a path, including NUL byte.  */
    SG_MAX_PATH = 128
};

/**
 * @brief Return codes for sg_file_load().
 */
enum {
    /** @brief Result code for success.  */
    SG_FILE_OK = 0,
    /** @brief Result code for failure.  */
    SG_FILE_ERROR = -1,
    /** @brief Result code indicating that the file was not loaded
        because it has not changed and the ::SG_IFCHANGED flag was
        set.  */
    SG_FILE_NOTCHANGED = -2
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
sg_path_norm(
    char *buf,
    const char *path,
    size_t pathlen,
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
sg_path_join(
    char *buf,
    const char *path,
    size_t pathlen,
    struct sg_error **err);

/**
 * @brief Flags for reading files.
 */
enum {
    /** @brief Only search the user data path.  */
    SG_USERONLY = 01,
    /** @brief Only search the application data path.  */
    SG_DATAONLY = 02,
    /** @brief Only load the file if it has changed.  */
    SG_IFCHANGED = 04
};

/**
 * @brief The contents of a file.
 *
 * This object is reference counted.
 */
struct sg_filedata {
    /**
     * @private @brief Reference count, do not modify.
     */
    sg_atomic_t refcount_;

    /**
     * @brief Pointer to buffer data.
     *
     * This buffer also contains one zero byte after the end of the
     * file.
     */
    void *data;

    /**
     * @brief Length of buffer data, not counting the extra zero at
     * the end.
     */
    size_t length;

    /**
     * @brief The relative path to the loaded file.
     *
     * The path is normalized, and includes the file extension.  This
     * is NUL-terminated.
     */
    const char *path;

    /**
     * @brief The length of the file's path, in bytes.
     *
     * Does not count the NUL terminator.
     */
    size_t pathlen;
};

/**
 * @brief Increment a file data reference count.
 */
void
sg_filedata_incref(struct sg_filedata *fbuf);

/**
 * @brief Decrement a file data reference count.
 */
void
sg_filedata_decref(struct sg_filedata *fbuf);

/**
 * @brief Identity of a file.
 *
 * This is entirely opaque.  It is just used to reload files when the
 * files change.
 */
struct sg_fileid {
    /**
     * @private @brief Private fields.
     */
    uint64_t f_[3];
};

/**
 * @brief Load a file into memory.
 *
 * When loading a file, this will search through the user and
 * application search paths for a file with the given relative path.
 * If a list of extensions is specified, then each of the extensions,
 * from a colon-delimited list, will be tried at each point in the
 * path.  Empty extensions are ignored.
 *
 * Example usage:
 *
 @code
 // Search for images/tree.png or images/tree.jpg
 struct sg_filedata *data;
 struct sg_fileid fid;
 int r = sg_file_load(&data, "images/tree", strlen("images/tree"),
                      0, "png:jpg", 1000*1000, &fid, NULL);
 if (r != SG_FILE_OK) {
     abort(); // error
 } else {
     // ... process the image ...
     sg_filedata_decref(data);
 }

 // Reload the file only if it has changed
 int r = sg_file_load(&data, "images/tree", strlen("images/tree"),
                      SG_IFCHANGED, "png:jpg", 1000*1000, &fid, NULL);
 if (r == SG_FILE_NOTCHANGED) {
     // File not changed
 } else if (r != SG_FILE_OK) {
     abort(); //error
 } else {
     // ... process the changed image ...
     sg_filedata_decref(data);
 }
 @endcode
 *
 * Earlier portions of the search path take priority first, extensions
 * are considered second.  For example, with search paths `"a"` and
 * `"b"`, path `"file"`, and extensions `"png:jpg"`, the following
 * paths are tried in order:
 *
 * - `a/file.png`
 * - `a/file.jpg`
 * - `b/file.png`
 * - `b/file.jpg`
 *
 * @param data A pointer to where the pointer to the file contents
 * will be stored.
 *
 * @param path The path to the file to load.  The path is relative to
 * the asset directories, and the path will be verified and sanitized.
 * The path does not need to be NUL-terminated.
 *
 * @param pathlen The length of the path, in bytes.
 *
 * @param flags Flags specifying how to load the file.
 *
 * @param extensions If not NULL, then this is a pointer to a list of
 * extensions, separated by colons.  The leading periods should be
 * omitted.  Each extension is added to the path, in turn, when
 * searching for the file.  If NULL, then the path is searched for as
 * is.
 *
 * @param maxsize The maximum size of the file to load.  If the file
 * is larger, then this will fail with the ::SG_ERROR_DATA domain.
 *
 * @param fileid If not NULL, a pointer to a record of the file's
 * identity.  If ::SG_IFCHANGED is set, the file will be compared
 * against the identity record, and the file will only be loaded if it
 * differs from the record.  In any case, if the file is loaded
 * successfully, its identity is stored in this record.
 *
 * @param err On failure, the error.
 *
 * @return ::SG_FILE_OK if successful, ::SG_FILE_ERROR if an error
 * occurred, ::SG_FILE_NOTCHANGED if ::SG_IFCHANGED was specified and
 * the file has not changed.
 */
int
sg_file_load(
    struct sg_filedata **data,
    const char *path,
    size_t pathlen,
    int flags,
    const char *extensions,
    size_t maxsize,
    struct sg_fileid *fileid,
    struct sg_error **err);

/**
 * @brief An output stream for writing data to a file.
 */
struct sg_writer;

/**
 * @brief Open a file for writing.
 *
 * The new data will atomically replace any existing file, once the
 * new file is complete.
 *
 * @param path The file's relative path.
 * @param pathlen The length of the path, in bytes.
 * @param err On failure, the error.
 * @return A pointer to an open file, or NULL on failure.
 */
struct sg_writer *
sg_writer_open(
    const char *path,
    size_t pathlen,
    struct sg_error **err);

/**
 * @brief Seek to a new file position.
 *
 * @param fp The file.
 * @param off The target offset.
 * @param whence The interpretation of the file offset.
 * @param err On failure, the error.
 * @return The new position, or a negative number if an error
 * occurred.
 */
int64_t
sg_writer_seek(
    struct sg_writer *fp,
    int64_t offset,
    int whence,
    struct sg_error **err);

/**
 * @brief Write data to a file.
 *
 * @param fp The file.
 * @param buf The source buffer.
 * @param amt The number of bytes to write.
 * @param err On failure, the error.
 * @return The number of bytes written, or a negative number if an
 * error occurred.
 */
int
sg_writer_write(
    struct sg_writer *fp,
    const void *buf,
    size_t amt,
    struct sg_error **err);

/**
 * @brief Commit changes to the file.
 *
 * Once this function is called, the only valid operation on the file
 * is sg_writer_close().  This will write out all remaining data to
 * disk, and atomically replace the old file with the new data.
 *
 * @param fp The file.
 * @return Zero if successful, nonzero if an error occurred.
 */
int
sg_writer_commit(
    struct sg_writer *fp,
    struct sg_error **err);

/**
 * @brief Close a file.
 *
 * If the output has not been committed, then closing the file will
 * erase the file.
 *
 * @param fp The file.
 */
void
sg_writer_close(
    struct sg_writer *fp);

#ifdef __cplusplus
}
#endif
#endif
