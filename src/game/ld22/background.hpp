#ifndef GAME_LD22_BACKGROUND_HPP
#define GAME_LD22_BACKGROUND_HPP
namespace LD22 {

class Background {
public:
    typedef enum {
        EMPTY,
        MOUNTAINS
    } Type;

    Background()
    { }
    virtual ~Background();    

    virtual void init() = 0;
    virtual void advance() = 0;
    virtual void draw(int delta) = 0;

    static Background *getBackground(Type t);
};

}
#endif
