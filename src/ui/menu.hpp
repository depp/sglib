#ifndef UI_MENU_HPP
#define UI_MENU_HPP
#include "SDL.h"
class Type;

class Menu {
public:
    Menu();
    ~Menu();
    void handleEvent(SDL_Event const &evt);
    void draw();

private:
    Type *title_;

    Menu(Menu const &);
    Menu &operator=(Menu const &);
};

#endif
