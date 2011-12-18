#ifndef GAME_LD22_AREA_HPP
#define GAME_LD22_AREA_HPP
#include "defs.hpp"
#include "client/texture.hpp"
#include <vector>
#include <stdio.h>
namespace LD22 {
class ScreenBase;
class Actor;

class Area {
    ScreenBase &m_screen;
    unsigned char m_tiles[TILE_HEIGHT][TILE_WIDTH];
    std::vector<Actor *> m_actors;

public:
    Area(ScreenBase &screen);
    ~Area();

    int getTile(int x, int y) const
    {
        if (x >= 0 && x < TILE_WIDTH && y >= 0 && y < TILE_HEIGHT)
            return m_tiles[y][x];
        return 0;
    }

    void setTile(int x, int y, int v)
    {
        if (x >= 0 && x < TILE_WIDTH && y >= 0 && y < TILE_HEIGHT)
            m_tiles[y][x] = v;
    }

    ScreenBase &screen()
    {
        return m_screen;
    }

    void addActor(Actor *a);
    void draw(int delta);
    void advance();
    void load();
};

}
#endif
