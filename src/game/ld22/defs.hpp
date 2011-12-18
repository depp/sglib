#ifndef GAME_LD22_DEFS_HPP
#define GAME_LD22_DEFS_HPP
namespace LD22 {

enum {
    InRight,
    InLeft,
    InUp,
    InDown,
    InThrow
};

static const int FRAME_TIME = 32;

static const int TILE_BITS = 5;
static const int TILE_SIZE = 1 << TILE_BITS;
static const int TILE_WIDTH = 24;
static const int TILE_HEIGHT = 15;
static const int SCREEN_WIDTH = TILE_SIZE * TILE_WIDTH;
static const int SCREEN_HEIGHT = TILE_SIZE * TILE_HEIGHT;
static const int STICK_WIDTH = 28, STICK_HEIGHT = 48;

static const int MAX_TILE = 16;
static const int SPEED_SCALE = 256;
static const int GRAVITY = SPEED_SCALE * 3/2;
static const int ITEM_SIZE = 48;

}
#endif
