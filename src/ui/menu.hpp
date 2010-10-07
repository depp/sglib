#ifndef UI_MENU_HPP
#define UI_MENU_HPP
#include "layer.hpp"
#include "type/rastertext.hpp"
#include <string>
#include <vector>
class RasterText;
namespace UI {
class Button;

class Menu : public Layer {
public:
    Menu();
    virtual ~Menu();
    virtual void handleEvent(SDL_Event const &evt);
    virtual void draw();

private:
    bool initted_;
    RasterText title_;
    Button *menu_[4];

    Menu(Menu const &);
    Menu &operator=(Menu const &);
};

}
#endif
