/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#ifndef SGPP_COLOR_HPP
#define SGPP_COLOR_HPP

struct Color {
    Color()
    {
        c[0] = 0;
        c[1] = 0;
        c[2] = 0;
    }

    Color(int r, int g, int b)
    {
        c[0] = r;
        c[1] = g;
        c[2] = b;
    }

    // These are the standard colors defined in HTML 4.
    static Color black()   { return Color(  0,   0,   0); }
    static Color silver()  { return Color(192, 192, 192); }
    static Color gray()    { return Color(128, 128, 128); }
    static Color white()   { return Color(255, 255, 255); }
    static Color maroon()  { return Color(128,   0,   0); }
    static Color red()     { return Color(255,   0,   0); }
    static Color purple()  { return Color(128,   0, 128); }
    static Color fuchsia() { return Color(255,   0, 255); }
    static Color green()   { return Color(  0, 128,   0); }
    static Color lime()    { return Color(  0, 255,   0); }
    static Color olive()   { return Color(128, 128,   0); }
    static Color yellow()  { return Color(255, 255,   0); }
    static Color navy()    { return Color(  0,   0, 128); }
    static Color blue()    { return Color(  0,   0, 255); }
    static Color teal()    { return Color(  0, 128, 128); }
    static Color aqua()    { return Color(  0, 255, 255); }

    unsigned char c[3];
};

#endif
