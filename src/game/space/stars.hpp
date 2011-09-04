#ifndef GAME_SPACE_STARS_HPP
#define GAME_SPACE_STARS_HPP
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
