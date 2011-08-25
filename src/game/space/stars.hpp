#ifndef STARS_H
#define STARS_H
namespace Space {

class Starfield {
public:
    Starfield();
    void draw(float xmin, float ymin, float xsize, float ysize);

    unsigned int seed;
    float parallax;
    float tileSize;
};

}
#endif
