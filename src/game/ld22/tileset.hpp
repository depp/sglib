#ifndef GAME_LD22_TILESET_HPP
#define GAME_LD22_TILESET_HPP
#include "client/texture.hpp"
#include "defs.hpp"
namespace LD22 {

class Tileset {
    Texture::Ref m_tex;

public:
    Tileset();
    ~Tileset();

    void drawTiles(const unsigned char t[TILE_HEIGHT][TILE_WIDTH],
                   int delta) const;
};

}
#endif
