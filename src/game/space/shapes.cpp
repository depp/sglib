#include "opengl.hpp"
#include "shapes.hpp"
namespace Space {

static const int PointCount = 6;
static const float points[PointCount + 1] = {
    0.0, 0.258819045103, 0.5, 0.707106781187,
    0.866025403784, 0.965925826289, 1.0
};

void drawCircle()
{
    for (int i = 0; i < PointCount; ++i)
        glVertex2f(points[PointCount-i], points[i]);
    for (int i = 0; i < PointCount; ++i)
        glVertex2f(-points[i], points[PointCount-i]);
    for (int i = 0; i < PointCount; ++i)
        glVertex2f(-points[PointCount-i], -points[i]);
    for (int i = 0; i < PointCount; ++i)
        glVertex2f(points[i], -points[PointCount-i]);
}

}
