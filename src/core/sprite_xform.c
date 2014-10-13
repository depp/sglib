/* Copyright 2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "sg/sprite.h"

sg_sprite_xform_t
sg_sprite_xform_compose(sg_sprite_xform_t x, sg_sprite_xform_t y)
{
    unsigned xi, yi, xm, ym;
    xi = (unsigned) x; xm = xi & 4u;
    yi = (unsigned) y; ym = yi & 4u;
    if (ym)
        xi *= 3u;
    return (sg_sprite_xform_t) ((xm ^ ym) | ((xi + yi) & 3));
}
