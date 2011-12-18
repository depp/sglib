#ifndef GAME_LD22_LEVEL_HPP
#define GAME_LD22_LEVEL_HPP
#include <vector>
#include <string>
#include "defs.hpp"
namespace LD22 {

struct Level {
    unsigned char tiles[TILE_HEIGHT][TILE_WIDTH];
    int playerx, playery;
    int background;

    // Get the pathname for the given level number
    static std::string pathForLevel(int num);

    // Create reasonable starting level
    void clear();

    // Load a level from disk
    void load(int num);

    // Save the level to disk
    void save(int num);
};

}
#endif
