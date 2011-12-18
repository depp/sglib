#ifndef GAME_LD22_BACKGROUND_HPP
#define GAME_LD22_BACKGROUND_HPP
namespace LD22 {

class Background {
public:
    int which;

    enum {
        EMPTY,
        MOUNTAINS
    };
    static const int COUNT = MOUNTAINS + 1;

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
