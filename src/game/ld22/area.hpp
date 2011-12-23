#ifndef GAME_LD22_AREA_HPP
#define GAME_LD22_AREA_HPP
#include "defs.hpp"
#include "client/texture.hpp"
#include <vector>
#include <stdio.h>
namespace LD22 {
class Screen;
class Actor;
class Thinker;

class Area {
    Screen &m_screen;
    unsigned char m_tiles[TILE_HEIGHT][TILE_WIDTH];
    std::vector<Actor *> m_actors, m_anew;
    std::vector<Thinker *> m_thinkers;
    int m_items, m_others, m_players, m_timer;

public:
    Area(Screen &screen);
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

    Screen &screen()
    {
        return m_screen;
    }

    void addActor(Actor *a);
    void addThinker(Thinker *a);
    void draw(int delta);
    void advance();
    void load();
    void clear();

    bool trace(int x1, int y1, int x2, int y2);

    const std::vector<Actor *> &actors()
    {
        return m_actors;
    }

    const std::vector<Thinker *> &thinkers()
    {
        return m_thinkers;
    }

    // Number of items / other is counted to determine winning and
    // losing the level
    void removeItem();
    void removeOther();
    void removePlayer();
};

}
#endif
