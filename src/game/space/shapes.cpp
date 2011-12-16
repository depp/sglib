#include "client/opengl.hpp"
#include "shapes.hpp"
namespace Space {

static const int PointCount = 6;
static const float points[PointCount + 1] = {
    0.0f, 0.258819045103f, 0.5f, 0.707106781187f,
    0.866025403784f, 0.965925826289f, 1.0f
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
