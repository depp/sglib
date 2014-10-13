/* Copyright 2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "sg/sprite.h"

void
sg_sprite_write(void *buffer, unsigned stride, struct sg_sprite sp,
                int x, int y)
{
    short *data;
    short tx0, tx1, ty0, ty1, rx0, rx1, ry0, ry1;

    tx0 = sp.x; tx1 = tx0 + sp.w;
    ty0 = sp.y; ty1 = ty0 + sp.h;
    rx0 = x - sp.cx, rx1 = rx0 + sp.w;
    ry0 = y - sp.cy, ry1 = ry0 + sp.h;

    data = buffer;
    data[0] = rx0; data[1] = ry0; data[2] = tx0; data[3] = ty0;
    data = (void *) ((unsigned char *) buffer + stride * 1);
    data[0] = rx1; data[1] = ry0; data[2] = tx1; data[3] = ty0;
    data = (void *) ((unsigned char *) buffer + stride * 2);
    data[0] = rx0; data[1] = ry1; data[2] = tx0; data[3] = ty1;
    data = (void *) ((unsigned char *) buffer + stride * 3);
    data[0] = rx0; data[1] = ry1; data[2] = tx0; data[3] = ty1;
    data = (void *) ((unsigned char *) buffer + stride * 4);
    data[0] = rx1; data[1] = ry0; data[2] = tx1; data[3] = ty0;
    data = (void *) ((unsigned char *) buffer + stride * 5);
    data[0] = rx1; data[1] = ry1; data[2] = tx1; data[3] = ty1;
}

void
sg_sprite_write2(void *buffer, unsigned stride, struct sg_sprite sp,
                 int x, int y, sg_sprite_xform_t xform)
{
    short *data;
    short vx0, vx1, vx2, vx3, vy0, vy1, vy2, vy3;
    short tx0, tx1, ty0, ty1, rx0, rx1, ry0, ry1;

    tx0 = sp.x; tx1 = tx0 + sp.w;
    ty0 = sp.y; ty1 = ty0 + sp.h;
    rx0 = -sp.cx, rx1 = rx0 + sp.w;
    ry0 = -sp.cy, ry1 = ry0 + sp.h;

    switch (xform) {
    case SG_X_NORMAL:
        vx0 = vx2 = x + rx0;
        vx1 = vx3 = x + rx1;
        vy0 = vy1 = y + ry0;
        vy2 = vy3 = y + ry1;
        break;

    case SG_X_ROTATE_90:
        vx2 = vx3 = x - ry1;
        vx0 = vx1 = x - ry0;
        vy2 = vy0 = y + rx0;
        vy3 = vy1 = y + rx1;
        break;

    case SG_X_ROTATE_180:
        vx3 = vx1 = x - rx1;
        vx2 = vx0 = x - rx0;
        vy3 = vy2 = y - ry1;
        vy1 = vy0 = y - ry0;
        break;

    case SG_X_ROTATE_270:
        vx1 = vx0 = x + ry0;
        vx3 = vx2 = x + ry1;
        vy1 = vy3 = y - rx1;
        vy0 = vy2 = y - rx0;
        break;

    case SG_X_FLIP_VERTICAL:
        vx0 = vx2 = x + rx0;
        vx1 = vx3 = x + rx1;
        vy3 = vy2 = y - ry1;
        vy1 = vy0 = y - ry0;
        break;

    case SG_X_TRANSPOSE_2:
        vx2 = vx3 = x - ry1;
        vx0 = vx1 = x - ry0;
        vy1 = vy3 = y - rx1;
        vy0 = vy2 = y - rx0;
        break;

    case SG_X_FLIP_HORIZONTAL:
        vx3 = vx1 = x - rx1;
        vx2 = vx0 = x - rx0;
        vy0 = vy1 = y + ry0;
        vy2 = vy3 = y + ry1;
        break;

    case SG_X_TRANSPOSE:
        vx1 = vx0 = x + ry0;
        vx3 = vx2 = x + ry1;
        vy2 = vy0 = y + rx0;
        vy3 = vy1 = y + rx1;
        break;

    default:
#ifdef __GCC__
        __builtin_unreachable();
#endif
        return;
    }

    data = buffer;
    data[0] = vx0; data[1] = vy0; data[2] = tx0; data[3] = ty0;
    data = (void *) ((unsigned char *) buffer + stride * 1);
    data[0] = vx1; data[1] = vy1; data[2] = tx1; data[3] = ty0;
    data = (void *) ((unsigned char *) buffer + stride * 2);
    data[0] = vx2; data[1] = vy2; data[2] = tx0; data[3] = ty1;
    data = (void *) ((unsigned char *) buffer + stride * 3);
    data[0] = vx2; data[1] = vy2; data[2] = tx0; data[3] = ty1;
    data = (void *) ((unsigned char *) buffer + stride * 4);
    data[0] = vx1; data[1] = vy1; data[2] = tx1; data[3] = ty0;
    data = (void *) ((unsigned char *) buffer + stride * 5);
    data[0] = vx3; data[1] = vy3; data[2] = tx1; data[3] = ty1;
}
