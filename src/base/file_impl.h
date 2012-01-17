/* Internal file / path subsystem interface.  */
#ifndef BASE_FILE_IMPL_H
#define BASE_FILE_IMPL_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
struct sg_file;

#if defined(_WIN32)
#define SG_PATH_UNICODE 1
#define SG_PATH_PATHSEP ';'
#define SG_PATH_DIRSEP '\\'
#else
#undef SG_PATH_UNICODE
#define SG_PATH_PATHSEP ':'
#define SG_PATH_DIRSEP '/'
#endif

#if defined(SG_PATH_UNICODE)
typedef wchar_t pchar;
#else
typedef char pchar;
#endif

/* Flags used by sg_path_getdir and sg_path_add (internal).  */
enum {
    /* Used internally.  Writable paths are searched first and they
       are created if necessary.  Files can only be written to
       writable paths.  */
    SG_PATH_WRITABLE = 1 << 0,
    /* Do not fail if the path does not exist.  Implied by
       SG_PATH_WRITABLE.  */
    SG_PATH_NODISCARD = 1 << 1,
    /* The path is relative to the executable's directory.  Otherwise
       it is relative to the current directory.  */
    SG_PATH_EXEDIR = 1 << 2
};

/* Open a file on the computer's filesystem.

   f: On return, if successful, an open file.

   path: The absolute path to open.

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

/* Get an absolute path for the given directory.  

   abspath, abslen: On return, if successful, a malloc'd buffer and
   its length containing the absolute path corresponding to relpath,
   rellen.  Not nul-terminated.  Must end with the directory separator
   appropriate for this platform.

   relpath, rellen: The relative path to check.  Interpretation of
   this is affected by the SG_PATH_EXEDIR flag.  Not nul-terminated.

   flags: Any combination of SG_PATH_NODISCARD and SG_PATH_EXEDIR.
   The SG_PATH_WRITABLE flag is ignored.

   return value: On success, a positive number is returned.  If the
   function fails because SG_PATH_NODISCARD is not set and the path
   does not exist or does not correspond to a directory, 0 is
   returned.  If any other error occurs, a negative number is
   returned.  */
int
sg_path_getdir(pchar **abspath, size_t *abslen,
               const char *relpath, size_t rellen,
               int flags);

#ifdef __cplusplus
}
#endif
#endif
