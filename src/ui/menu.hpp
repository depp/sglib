#ifndef UI_MENU_HPP
#define UI_MENU_HPP
#include "container.hpp"
#include "screen.hpp"
#include "button.hpp"
namespace UI {

class Menu : public Screen {
public:
    Menu();
    virtual ~Menu();

    virtual void handleEvent(Event const &evt);
    virtual void draw(unsigned int ticks);

private:
    bool initted_;
    Container ui_;
    Button menu_[4];

    Menu(Menu const &);
    Menu &operator=(Menu const &);
};

}
#endif
