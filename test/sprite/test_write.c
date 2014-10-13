/* Copyright 2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "sg/sprite.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NVERTEX 6

static void
vert_xform(short (*vert)[4], sg_sprite_xform_t xform, int x, int y)
{
    int i, j, xfi, vx, vy;
    xfi = (int) xform;
    /* Rotations */
    for (i = 0; i < (xfi & 3); i++) {
        for (j = 0; j < NVERTEX; j++) {
            vx = vert[j][0];
            vy = vert[j][1];
            vert[j][0] = -vy;
            vert[j][1] = vx;
        }
    }
    /* Vertical mirroring */
    if ((xfi & 4) != 0) {
        for (j = 0; j < NVERTEX; j++)
            vert[j][1] *= -1;
    }
    /* Translation */
    for (j = 0; j < NVERTEX; j++) {
        vert[j][0] += x;
        vert[j][1] += y;
    }
}

static void
assert_equal(short (*const x)[4], short (*const y)[4])
{
    int i, j;
    for (i = 0; i < NVERTEX; i++) {
        for (j = 0; j < 4; j++) {
            if (x[i][j] != y[i][j]) {
                fputs("FAIL\n", stderr);
                exit(1);
            }
        }
    }
}

int
main(int argc, char **argv)
{
    static const short SQUARE[6][2] = {
        { 0, 0 },
        { 1, 0 },
        { 0, 1 },
        { 0, 1 },
        { 1, 0 },
        { 1, 1 }
    };
    short vert1[NVERTEX][4], vert2[NVERTEX][4];
    struct sg_sprite sp;
    int x, y, xfi, i;
    sg_sprite_xform_t xform;

    (void) argc;
    (void) argv;

    /* Reduced chances of errors canceling out */
    x = 128;
    y = 64;
    sp.x = 32;
    sp.y = 16;
    sp.w = 8;
    sp.h = 4;
    sp.cx = 2;
    sp.cy = 1;

    for (xfi = 0; xfi < 8; xfi++) {
        xform = (sg_sprite_xform_t) xfi;
        for (i = 0; i < NVERTEX; i++) {
            vert1[i][0] = SQUARE[i][0] * sp.w - sp.cx;
            vert1[i][1] = SQUARE[i][1] * sp.h - sp.cy;
            vert1[i][2] = SQUARE[i][0] * sp.w + sp.x;
            vert1[i][3] = SQUARE[i][1] * sp.h + sp.y;
        }
        vert_xform(vert1, xform, x, y);

        if (xform == SG_X_NORMAL) {
            sg_sprite_write(vert2, sizeof(*vert2), sp, x, y);
            assert_equal(vert1, vert2);
        }
        sg_sprite_write2(vert2, sizeof(*vert2), sp, x, y, xform);
        assert_equal(vert1, vert2);
    }

    fputs("ok\n", stderr);
    return 0;
}
