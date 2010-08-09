#ifndef UI_MENU_HPP
#define UI_MENU_HPP
#include "uilayer.hpp"
class Type;

class Menu : public UILayer {
public:
    Menu();
    virtual ~Menu();
    virtual void handleEvent(SDL_Event const &evt);
    virtual void draw();

private:
    Type *title_;

    Menu(Menu const &);
    Menu &operator=(Menu const &);
};

#endif
