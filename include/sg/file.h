#ifndef BASE_FILE_H
#define BASE_FILE_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* The LFILE (low-level file) interface implements simple features
   like reading an entire file into memory or writing a file
   atomically.  */
struct sg_error;

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

/* A buffer of data read from a file.  Can be copied or moved.  Do not
   free manually.  */
struct sg_buffer {
    /* The requested data.  */
    void *data;
    size_t length;
};

void
sg_buffer_destroy(struct sg_buffer *fbuf);

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

/* Read the contents of a file, starting with the current position.
   Return 0 for success, -1 for failure, or 1 if the amount of data
   read would exceed maxsize (which sets the error).  The buffer will
   contain an extra zero byte past the end.  */
int
sg_file_readall(struct sg_file *f, struct sg_buffer *fbuf, size_t maxsize);

/* Get the data from the file at the given path and return 0, return
   -1 for failure, or return 1 if the amount of data read would exceed
   maxsize (which sets the error).  The buffer will contain an extra
   zero byte past the end.  */
int
sg_file_get(const char *path, size_t pathlen, int flags,
            const char *extensions,
            struct sg_buffer *fbuf, size_t maxsize, struct sg_error **e);

#ifdef __cplusplus
}
#endif
#endif
