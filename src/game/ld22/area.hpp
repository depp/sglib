#ifndef GAME_LD22_AREA_HPP
#define GAME_LD22_AREA_HPP
#include "defs.hpp"
#include "client/texture.hpp"
#include <vector>
namespace LD22 {
class Actor;

/* One "screen" of level area.  */
class Area {
public:
    static const int SCALE = 5;

private:
    unsigned char m_tile[TILE_HEIGHT][TILE_WIDTH];
    std::vector<Actor *> m_actors;
    Texture::Ref m_tex;

public:
    Area();
    ~Area();

    int getTile(int x, int y) const
    {
        if (x >= 0 && x < TILE_WIDTH && y >= 0 && y < TILE_HEIGHT)
            return m_tile[y][x];
        return 0;
    }

    void setTile(int x, int y, int v)
    {
        if (x >= 0 && x < TILE_WIDTH && y >= 0 && y < TILE_HEIGHT)
            m_tile[y][x] = v;
    }

    void addActor(Actor *a);

    void draw(int delta);
    void advance();
};

}
#endif
