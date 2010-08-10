#ifndef UI_MENU_HPP
#define UI_MENU_HPP
#include "layer.hpp"
#include <string>
#include <vector>
class Type;
namespace UI {

class Menu : public Layer {
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

}
#endif
