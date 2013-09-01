/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#ifndef SGPP_UI_GEOMETRY_HPP
#define SGPP_UI_GEOMETRY_HPP
namespace UI {

struct Point {
    Point(int xx, int yy) : x(xx), y(yy) { }

    int x, y;
};

struct Rect {
    int x, y, width, height;

    bool contains(int px, int py) const
    {
        return px >= x && (px - x) < width
            && py >= y && (py - y) < height;
    }
};

}
#endif
