/* Copyright 2012-2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
/* Internal file / path subsystem interface.  */
#include <stddef.h>
#include <stdint.h>
#include "sg/cvar.h"
struct sg_error;
struct sg_filedata;
struct sg_fileid;
struct sg_writer;

/* Return codes for sg_reader_open() */
enum {
    SG_FILE_NOTFOUND = -2,
};

#define SG_PATH_MAXEXTS 8
#define SG_PATH_MAXCOUNT 8

#if defined _WIN32

#include <wchar.h>
typedef wchar_t pchar;
#define SG_PATH_PATHSEP ';'
#define SG_PATH_DIRSEP '\\'
#define pmemcpy wmemcpy
#define pmemchr wmemchr

#else

#include <string.h>
typedef char pchar;
#define SG_PATH_PATHSEP ':'
#define SG_PATH_DIRSEP '/'
#define pmemcpy memcpy
#define pmemchr memchr

#endif

/* A search path.  */
struct sg_path {
    pchar *path;
    size_t len;
};

/* List of all search paths.  This should never be modified, except
   once at startup by sg_path_init().  The first path is always the
   user search path, where new files can be written.  Paths always end
   with the directory separator character, so you can concatenate file
   names and relative paths directly onto the end.  */
struct sg_paths {
    struct sg_path *path;
    unsigned pathcount;
    unsigned maxlen;
    struct sg_cvar_string cvar[2];
};

/* Global list of search paths.  */
extern struct sg_paths sg_paths;

/* The path where game data is stored alongside the application, if it
   is stored alongside the application.  */
#define SG_PATH_DATA "Data"
/* The path where user data is stored alongside the application, if it
   is stored alongside the application.  */
#define SG_PATH_USER "User"

#if defined _WIN32
struct sg_reader {
    void *handle;
};
#else
struct sg_reader {
    int fdes;
};
#endif

/* Returns 0 on success, -1 if the file was not found, and -2 for
   other errors.  */
int
sg_reader_open(
    struct sg_reader *fp,
    const pchar *path,
    struct sg_error **err);

/* Returns 0 for success, -1 for error.  */
int
sg_reader_getinfo(
    struct sg_reader *fp,
    int64_t *length,
    struct sg_fileid *fileid,
    struct sg_error **err);

/* Returns number of bytes read, or -1 for error.  */
int
sg_reader_read(
    struct sg_reader *fp,
    void *buf,
    size_t bufsz,
    struct sg_error **err);

void
sg_reader_close(
    struct sg_reader *fp);

/* Returns NULL on error.  */
struct sg_filedata *
sg_reader_load(
    struct sg_reader *fp,
    size_t size,
    const char *path,
    size_t pathlen,
    struct sg_error **err);

/* Copy the normalized path 'src' to the destination 'dst', converting
   normalized directory separators to platform directory separators
   (e.g., turn slashes backwards on Windows).  */
void
sg_path_copy(pchar *dest, const char *src, size_t len);

/* Create the parent directory containing a file, recursively if
   necessary.  The path data will be modified, but returned to its
   original value on success (with normalized path separators).
   Returns SG_FILE_OK if the directory was created or if it already
   exists.  Returns SG_FILE_ERROR if an error occurs.  Returns
   SG_FILE_NOTFOUND if the directory does not exist and could not be
   created.  */
int
sg_path_mkpardir(pchar *path, struct sg_error **err);

/* Get the default user data path.  Returns the path length, or 0 for
   failure.  */
size_t
sg_path_getuserpath(pchar *buf, size_t buflen);

/* Get the default app data path.  Returns the path length, or 0 for
   failure.  */
size_t
sg_path_getdatapath(pchar *buf, size_t buflen);

/* Get the path where a new file would be created.  Create the
   directory containing the file, if necessary.  The resulting buffer
   must be freed with free() afterwards.  Returns NULL on error.

   This is used to interact with APIs that create files, but where
   it's more convenient to pass a filename rather than callbacks.  */
pchar *
sg_file_createpath(const char *path, size_t pathlen,
                   struct sg_error **err);
