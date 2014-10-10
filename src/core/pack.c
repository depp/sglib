/* Copyright 2013-2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "sg/error.h"
#include "sg/pack.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

/*
  Rectangle packing

  We use a simple algorithm which is not terribly inefficient and not
  terribly slow.

  For any enclosing size, try to pack the rectangles inside the
  enclosing rectangle, with the widest packing rectangle first.  Try
  to place each rectangle below the previous, or at the top if there
  is no space remaining below the previous rectangle.  Rectangles get
  placed at the leftmost available position once the vertical position
  is selected, although the available horizontal positions (tracked in
  the "frontier" array) are not tracked precisely.
*/

struct sg_pack_ref {
    unsigned index;
    unsigned short w, h;
    unsigned short x, y;
};

static int sg_pack_ref_compare(const void *p1, const void *p2)
{
    const struct sg_pack_ref *r1 = p1, *r2 = p2;
    if (r1->w != r2->w)
        return r1->w > r2->w ? -1 : 1;
    if (r1->h != r2->h)
        return r1->h > r2->h ? -1 : 1;
    return 0;
}

int
sg_pack(struct sg_pack_rect *rect, unsigned rectcount,
        struct sg_pack_size *size, const struct sg_pack_size *maxsize,
        struct sg_error **err)
{
    struct sg_pack_ref *ref;
    unsigned i, j, wi, hi, pw, ph, rw, rh, maxw, maxh, ypos, xpos;
    unsigned total_area, rect_area, pack_area, best_area, frontier_size;
    int success, pack_nonsquare, best_nonsquare;
    unsigned short *frontier;

    if (rectcount == 0) {
        size->width = 1;
        size->height = 1;
        return 1;
    }

    maxw = maxsize->width;
    maxh = maxsize->height;

    ref = malloc(sizeof(*ref) * rectcount);
    if (!ref) {
        sg_error_nomem(err);
        return -1;
    }
    total_area = 0;
    for (i = 0; i < rectcount; i++) {
        rw = rect[i].w;
        rh = rect[i].h;
        rect_area = rw * rh;
        ref[i].index = i;
        ref[i].w = rect[i].w;
        ref[i].h = rect[i].h;
        if (rect_area > (unsigned) -1 - total_area ||
            rw > maxw || rh > maxh) {
            free(ref);
            return 0;
        }
        total_area += rect_area;
    }
    qsort(ref, rectcount, sizeof(*ref), sg_pack_ref_compare);
    /*
    for (i = 0; i < rectcount; i++)
        printf("%3d %3d %3d\n", ref[i].index, ref[i].w, ref[i].h);
    */

    success = 0;
    frontier_size = 0;
    frontier = NULL;
    for (hi = 0; hi < 16; hi++) {
        ph = 1u << hi;
        if (ph > maxh)
            break;
        for (wi = 0; wi < 16; wi++) {
            pw = 1u << wi;
            if (pw > maxw)
                break;
            pack_area = pw * ph;
            if (total_area > pack_area)
                continue;
            pack_nonsquare = abs(wi - hi);
            if (success &&
                (pack_area > best_area ||
                 (pack_area == best_area && pack_nonsquare > best_nonsquare)))
                break;

            if (frontier_size < ph) {
                free(frontier);
                frontier_size = ph;
                frontier = calloc(frontier_size, sizeof(*frontier));
                if (!frontier) {
                    free(ref);
                    sg_error_nomem(err);
                    return -1;
                }
            } else {
                memset(frontier, 0, sizeof(*frontier) * ph);
            }

            ypos = 0;
            for (i = 0; i < rectcount; i++) {
                rw = ref[i].w;
                rh = ref[i].h;
                if (ypos + rh > ph)
                    ypos = 0;
                while (ypos + rh <= ph && frontier[ypos] + rw > pw)
                    ypos++;
                if (ypos + rh > ph)
                    break;
                xpos = frontier[ypos];
                for (j = 0; j < ypos + rh; j++) {
                    if (frontier[j] < xpos + rw)
                        frontier[j] = xpos + rw;
                }
                assert(ypos <= ph - rh);
                assert(xpos <= pw - rw);
                ref[i].x = xpos;
                ref[i].y = ypos;
                ypos += rh;
            }

            if (i == rectcount) {
                best_area = pack_area;
                best_nonsquare = pack_nonsquare;
                size->width = pw;
                size->height = ph;
                for (i = 0; i < rectcount; i++) {
                    j = ref[i].index;
                    rect[j].x = ref[i].x;
                    rect[j].y = ref[i].y;
                }
                success = 1;
                break;
            }
        }
    }

    free(frontier);
    free(ref);
    return success;
}
