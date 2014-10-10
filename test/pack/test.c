/* Copyright 2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "sg/error.h"
#include "sg/pack.h"
#include "sg/rand.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static struct sg_rand_state state = {
    1234567, 7654321, 1234321
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

static void
test_pack(unsigned count, unsigned rectw, unsigned recth,
          unsigned packw, unsigned packh)
{
    unsigned i, j, area, pack_area;
    unsigned short *dim;
    struct sg_pack_rect *rect;
    struct sg_pack_size sz, maxsz;
    int r;

    maxsz.width = packw;
    maxsz.height = packh;

    area = 0;
    dim = xmalloc(sizeof(*dim) * count * 2);
    rect = xmalloc(sizeof(*rect) * count);
    for (i = 0; i < count; i++) {
        rect[i].w = dim[i*2+0] = (sg_irand(&state) % rectw) + 1;
        rect[i].h = dim[i*2+1] = (sg_irand(&state) % recth) + 1;
        rect[i].x = 0xffff;
        rect[i].y = 0xffff;
        area += (unsigned) rect[i].w * (unsigned) rect[i].h;
    }

    sz.width = 0xffff;
    sz.height = 0xffff;
    r = sg_pack(rect, count, &sz, &maxsz, NULL);
    for (i = 0; i < count; i++) {
        assert(rect[i].w == dim[i*2+0]);
        assert(rect[i].h == dim[i*2+1]);
    }
    if (r < 0) {
        puts("Error");
        exit(1);
    } else if (r > 0) {
        assert(sz.width > 0);
        assert((sz.width & (sz.width - 1)) == 0);
        assert(sz.height > 0);
        assert((sz.height & (sz.height - 1)) == 0);
        for (i = 0; i < count; i++) {
            assert(rect[i].w <= sz.width);
            assert(rect[i].x <= sz.width - rect[i].w);
            assert(rect[i].h <= sz.height);
            assert(rect[i].y <= sz.height - rect[i].h);
            for (j = i + 1; j < count; j++) {
                if (rect[i].x < rect[j].x + rect[j].w &&
                    rect[i].y < rect[j].y + rect[j].h &&
                    rect[j].x < rect[i].x + rect[i].w &&
                    rect[j].y < rect[i].y + rect[i].h) {
                    puts("Collision");
                    printf("%d x %d @ %d, %d\n",
                           rect[i].w, rect[i].h, rect[i].x, rect[i].y);
                    printf("%d x %d @ %d, %d\n",
                           rect[j].w, rect[j].h, rect[j].x, rect[j].y);
                    exit(1);
                }
            }
        }

        pack_area = (unsigned) sz.width * (unsigned) sz.height;
        printf("Rects: %u; size: %d x %d; efficiency: %0.4f\n",
               count, sz.width, sz.height,
               (double) area / (double) pack_area);
    }

    free(dim);
    free(rect);
}

int
main(int argc, char **argv)
{
    int i;
    (void) argc;
    (void) argv;

    test_pack(1, 100, 100, 1024, 1024);
    for (i = 0; i < 50; i++)
        test_pack(100, 100, 100, 1024, 1024);
    for (i = 0; i < 10; i++)
        test_pack(10000, 32, 32, 4096, 4096);

    return 0;
}
