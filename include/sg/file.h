/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#ifndef SG_FILE_H
#define SG_FILE_H
#include "libpce/atomic.h"
#include "libpce/attribute.h"
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* The LFILE (low-level file) interface implements simple features
   like reading an entire file into memory or writing a file
   atomically.  */
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

enum {
    /* Access modes (sg_file_open only) */
    SG_RDONLY = 00,
    SG_WRONLY = 01,

    /* Ignore files in archives */
    SG_NO_ARCHIVE = 02,

    /* Ignore files in read-only paths */
    SG_LOCAL = 04,

    /* Alias for files which can be overwritten */
    SG_WRITABLE = SG_NO_ARCHIVE | SG_LOCAL,

    /* Maximum length of an internal path, including NUL byte */
    SG_MAX_PATH = 128
};

/* Initialize the path subsystem.  */
void
sg_path_init(void);

/* Normalize a path.  Returns the length of the result, or -1 if the
   path is not legal, which sets the error.  The buffer must be
   SG_MAX_PATH long.  The resulting path will use '/' as a path
   separator, will not start with '/', and each component will be a
   non-empty lower-case string containing only alphanumeric characters
   and "-", "_", and ".".  No component will start or end with a
   period, and no component will be a reserved device name on Windows.
   If the input path refers to a directory, the output will either end
   with '/' or be empty.  */
int
sg_path_norm(char *buf, const char *path, size_t pathlen,
             struct sg_error **err);

/* Join a relative path to a base path, and normalize the result.  The
   base path must already be in the buffer.  If the base path is
   nonempty but does not end with a slash, then the resulting path is
   relative to the directory containing the base path.  Otherwise, the
   resulting path is relative to the base path.  This is the same
   logic used for relative URLs, but not the same logic used by
   e.g. Python's os.path.join().  Has the same results as
   sg_path_norm().  */
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
     * @private @brief Reference count.
     */
    pce_atomic_t refcount;

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
 * @private @brief Free a buffer of data.
 */
void
sg_buffer_free_(struct sg_buffer *fbuf);

/**
 * @brief Increment a buffer's reference count.
 */
PCE_INLINE void
sg_buffer_incref(struct sg_buffer *fbuf)
{
    pce_atomic_inc(&fbuf->refcount);
}

/**
 * @brief Decrement a buffer's reference count.
 */
PCE_INLINE void
sg_buffer_decref(struct sg_buffer *fbuf)
{
    int c = pce_atomic_fetch_add(&fbuf->refcount, -1);
    if (c == 1)
        sg_buffer_free_(fbuf);
}

/* An abstract file open for reading.  Do not copy this structure.
   The methods 'read' and 'close' must be set.  The 'length' and
   'seek' methods may be both NULL or both set.  The 'map' method may
   be NULL or may be set.  */
struct sg_file {
    /* Reference count, you may use this if you want.  It is
       initialized to 1 but never examined.  */
    unsigned refcount;

    /* Return the number of bytes read for success, 0 for EOF, and -1
       for errors.  */
    int     (*read)  (struct sg_file *f, void *buf, size_t amt);

    /* Return the number of bytes written for success or -1 for
       error.  */
    int     (*write) (struct sg_file *f, const void *buf, size_t amt);

    /* Close the file.  Return 0 if successful, and -1 if there were
       any pending IO errors.  In either case, the file will be
       closed.  */
    int     (*close) (struct sg_file *f);

    /* Free the file structure.  Closes the file if necessary.  */
    void    (*free)  (struct sg_file *f);

    /* Get the file length or return -1 for an error.  */
    int64_t (*length)(struct sg_file *f);

    /* Seek and return the new offset, or return -1 for an error.  */
    int64_t (*seek)  (struct sg_file *f, int64_t off, int whence);

    /*
    void    (*map)   (struct sg_file *f, struct sg_buffer *buf,
                      int64_t off, size_t len);
    */

    struct sg_error *err;

};

/* Open a open a regular file, or return NULL for failure.

   If extensions is not NULL, it is a list of extensions to search
   separated by ":".  Extensions do not begin with a period.  If the
   given path already starts with one of the extensions in the list,
   it is stripped and that extension is tried first.  Earlier search
   paths override later search paths.

   For example, with search path "a:b", path "file", and extensions
   "png:jpg", the following paths are tried in order:

   > a/file.png a/file.jpg b/file.png b/file.jpg

   The list of extensions is assumed to be safe and lower case.  */
struct sg_file *
sg_file_open(const char *path, size_t pathlen, int flags,
             const char *extensions, struct sg_error **e);

/**
 * @brief Get the contents of a file starting from the current
 * position.
 *
 * @param f The file.
 *
 * @param maxsize Maximum file size.  If the file is larger than this,
 * the operation will fail with the ::SG_ERROR_DATA domain.
 *
 * @return On success, a reference to a buffer containing the
 * requested data.  On failure, `NULL`, and an error will be set on
 * the file handle.
 */
struct sg_buffer *
sg_file_readall(struct sg_file *f, size_t maxsize);

/**
 * @brief Get the contents of a file given its path.
 *
 * @param path File path, relative to the game root directory.  Does
 * not need to be NUL-terminated, sanitized, or normalized.
 *
 * @param pathlen Length of the file path.
 *
 * @param flags File mode flags.  ::SG_RDONLY is assumed.
 *
 * @param extensions A NUL-terminated list of extensions to try,
 * separated by `':'`.  The extensions should not start with `'.'`.
 * For example, `"png:jpg"`.
 *
 * @param maxsize Maximum file size.  If the file is larger than this,
 * the operation will fail with the ::SG_ERROR_DATA domain.
 *
 * @param e On failure, the error.
 *
 * @return On success, a reference to a buffer containing the
 * requested data.  On failure, `NULL`.
 */
struct sg_buffer *
sg_file_get(const char *path, size_t pathlen, int flags,
            const char *extensions,
            size_t maxsize, struct sg_error **e);

#ifdef __cplusplus
}
#endif
#endif
