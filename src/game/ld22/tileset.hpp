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
    Bomb,
    Bang,
    Question
};

static const int MAX_WIDGET = Question;
};

class Tileset {
    Texture::Ref m_tile, m_stick, m_widget, m_end;

public:
    Tileset();
    ~Tileset();

    void drawTiles(const unsigned char t[TILE_HEIGHT][TILE_WIDTH],
                   int delta) const;

    void drawStick(int x, int y, int frame, bool isOther);

    void drawWidget(int x, int y, int which, float scale);

    void loadEnd();

    void drawEnd(int x, int y, int which, float scale);
};

}
#endif
