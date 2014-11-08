/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
/* Internal file / path subsystem interface.  */
#include <stddef.h>
struct sg_file;
struct sg_error;

#if defined(_WIN32)

typedef wchar_t pchar;
#define SG_PATH_PATHSEP ';'
#define SG_PATH_DIRSEP '\\'
#define pathrchr wcsrchr

#else

typedef char pchar;
#define SG_PATH_PATHSEP ':'
#define SG_PATH_DIRSEP '/'
#define pathrchr strrchr

#endif

/* Create the parent directory containing a file, recursively if
   necessary.  Returns 0 if the directory was created or if it already
   exists.  Returns -1 if an error occurred.  */
int
sg_file_mkpardir(const pchar *path, struct sg_error **err);

/* Get the path where a new file would be created.  Create the
   directory containing the file, if necessary.  The resulting buffer
   must be freed with free() afterwards.  Returns NULL on error.

   This is used to interact with APIs that create files, but where
   it's more convenient to pass a filename rather than callbacks.  */
pchar *
sg_file_createpath(const char *path, size_t pathlen,
                   struct sg_error **err);

/* Open a file on the computer's filesystem.

   f: On return, if successful, an open file.

   path: The path to open.

   flags: Open for writing if SG_WRONLY is set, otherwise open for
   reading.  SG_RDONLY is 0, so don't check for it.  Other flags
   should be ignored.

   e: The error object

   return value: On success, a positive number is returned.  If the
   file is not found and SG_WRONLY is not set, then return 0 and do
   not set either f or e.  If another type of error occurs, then
   return -1 and set e.  */
int
sg_file_tryopen(struct sg_file **f, const pchar *path, int flags,
                struct sg_error **e);

/* Get the path to the application.  For Linux and Windows, this means
   the executable files.  On Mac OS X, it means the path to the
   application bundle.  On success, return 1.  On failure, return 0.
   The path will be NUL-terminated.  */
int
sg_path_getexepath(pchar *path, size_t len);

/* Return 1 if the directory exists and is readable.  Return 0 if the
   directory does not exist or is not readable.  In either case, log
   an INFO message.  */
int
sg_path_checkdir(const pchar *path);
