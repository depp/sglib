#ifndef GAME_LD22_BACKGROUND_HPP
#define GAME_LD22_BACKGROUND_HPP
namespace LD22 {

class Background {
public:
    const int which;

    enum {
        EMPTY,
        MOUNTAINS,
        CITY,
        COUNT
    };
    static const int MAX = COUNT - 1;

    Background(int which_)
        : which(which_)
    { }

    virtual ~Background();

    virtual void init() = 0;
    virtual void advance() = 0;
    virtual void draw(int delta) = 0;

    static Background *getBackground(int which);
};

}
#endif
