#ifndef GAME_LD22_TILESET_HPP
#define GAME_LD22_TILESET_HPP
#include "client/texture.hpp"
#include "defs.hpp"
namespace LD22 {

namespace Widget {
enum {
    ThoughtLeft,
    ThoughtRight,
    SpeakLeft,
    SpeakRight,
    Star,
    Key1Left,
    Key1Right,
    Heart,
    Bomb
};

static const int MAX_WIDGET = Bomb;
};

class Tileset {
    Texture::Ref m_tile, m_stick, m_widget;

public:
    Tileset();
    ~Tileset();

    void drawTiles(const unsigned char t[TILE_HEIGHT][TILE_WIDTH],
                   int delta) const;

    void drawStick(int x, int y, int frame, bool isOther);

    void drawWidget(int x, int y, int which, float scale);
};

}
#endif
