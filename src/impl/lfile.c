#define _FILE_OFFSET_BITS 64

#include "lfile.h"
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

int
lfile_readall(struct lfile_contents *p, const char *path, size_t maxsize)
{
    int fd, e, r;
    struct stat st;
    void *data;
    size_t sz, pos;
    ssize_t amt;

    fd = open(path, O_RDONLY);
    if (fd < 0)
        return errno;
    r = fstat(fd, &st);
    if (r) {
        e = errno;
        close(fd);
        return e;
    }
    if (!S_ISREG(st.st_size)) {
        close(fd);
        return EACCES;
    }
    if (st.st_size > maxsize) {
        close(fd);
        return -1;
    }

    sz = st.st_size;
    data = malloc(sz + 1);
    if (!data) {
        close(fd);
        return ENOMEM;
    }

    pos = 0;
    while (pos < sz) {
        amt = read(fd, (unsigned char *) data + pos, sz - pos);
        if (amt > 0) {
            pos += amt;
        } else if (amt < 0) {
            e = errno;
            close(fd);
            free(data);
            return e;
        } else {
            break;
        }
    }
    ((unsigned char *) data)[pos] = '\0';

    p->data = data;
    p->length = sz;
    close(fd);
    return 0;
}

void
lfile_read_destroy(struct lfile_contents *p)
{
    free(p->data);
}
