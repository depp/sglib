#ifndef IMPL_LFILE_H
#define IMPL_LFILE_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
/* The LFILE (low-level file) interface implements simple features
   like reading an entire file into memory or writing a file
   atomically.  */

struct lfile_contents {
    void *data;
    size_t length;
};

/* Read a file into memory and return 0, or return an error code.  If
   the file exceeds the maximum size specified, then return -1.  If
   the file is not a regular file, EACCES is returned.  The resulting
   buffer will have one extra byte beyond the end set to 0.  */
int
lfile_readall(struct lfile_contents *p, const char *path, size_t maxsize);

#ifdef __cplusplus
}
#endif
#endif
