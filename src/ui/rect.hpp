#ifndef UI_RECT_HPP
#define UI_RECT_HPP
namespace UI {

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
