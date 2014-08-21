#include "sg/error.h"
#include "sg/file.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *const DATA[][2] = {
    { "abc", "abc" },
    { "abc/def", "abc/def" },
    { "fx/stereo", "fx/stereo" },
    { "/abc", "abc" },
    { "./abc", "abc" },
    { "abc/../def", "def" },
    { "/abc/def/ghi/../../jkl", "abc/jkl" },
    { "///abc/.///def/./ghi/./.././//../jkl", "abc/jkl" },
};

static void
die(const char *reason)
{
    fprintf(stderr, "error: %s\n", reason);
    exit(1);
}

static void *
xmalloc(size_t sz)
{
    void *p;
    if (!sz)
        return NULL;
    p = malloc(sz);
    if (!p)
        die("out of memory");
    return p;
}

int
main(int argc, char **argv)
{
    int i, n, r;
    const char *ix, *iy;
    char *mx, *my;
    size_t sz;
    struct sg_error *err;

    (void) argc;
    (void) argv;

    n = sizeof(DATA) / sizeof(*DATA);
    for (i = 0; i < n; i++) {
        ix = DATA[i][0];
        iy = DATA[i][1];
        sz = strlen(ix);
        mx = xmalloc(sz);
        memcpy(mx, ix, sz);
        my = xmalloc(SG_MAX_PATH);
        err = NULL;
        r = sg_path_norm(my, mx, sz, &err);
        if (r < 0) {
            if (!err || err->domain != &SG_ERROR_INVALPATH)
                die("bad error");
            sg_error_clear(&err);
            if (iy) {
                fprintf(stderr, "test %d: expected result, got error\n", i);
                exit(1);
            }
        } else {
            sz = strlen(iy);
            if (memcmp(iy, my, sz + 1)) {
                fprintf(stderr, "test %d: expected %s, got %s\n", i, iy, my);
                exit(1);
            }
            if (r != (int) sz) {
                fprintf(stderr, "test %d: wrong result\n", i);
                exit(1);
            }
        }
        free(mx);
        free(my);
    }

    return 0;
}
