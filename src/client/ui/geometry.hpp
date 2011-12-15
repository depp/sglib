#ifndef UI_GEOMETRY_HPP
#define UI_GEOMETRY_HPP
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
