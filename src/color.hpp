#ifndef COLOR_HPP
#define COLOR_HPP

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

    // Taken from HTML 4
    static const Color black, silver, gray, white, maroon, red,
        purple, fuchsia, green, lime, olive, yellow, navy, blue,
        teal, aqua;

    unsigned char c[3];
};

#endif
