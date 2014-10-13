/* Copyright 2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "sg/sprite.h"
#include <stdio.h>
#include <stdlib.h>

/*
  Vertexes on a square are numbered:

  2 3
  0 1

  This table maps each location to its new location, when modified by
 one of the sprite transformations.
*/
static const int XFORM[8][4] = {
    { 0, 1, 2, 3 },
    { 1, 3, 0, 2 },
    { 3, 2, 1, 0 },
    { 2, 0, 3, 1 },
    { 2, 3, 0, 1 },
    { 3, 1, 2, 0 },
    { 1, 0, 3, 2 },
    { 0, 2, 1, 3 }
};

int
main(int argc, char **argv)
{
    int x, y, z, i;

    (void) argc;
    (void) argv;

    for (x = 0; x < 8; x++) {
        for (y = 0; y < 8; y++) {
            z = (int) sg_sprite_xform_compose(
                (sg_sprite_xform_t) x, (sg_sprite_xform_t) y);
            for (i = 0; i < 4; i++) {
                if (XFORM[x][XFORM[y][i]] != XFORM[z][i]) {
                    fputs("FAIL\n", stderr);
                    exit(1);
                }
            }
        }
    }

    return 0;
}
