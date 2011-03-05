#ifndef UI_MENU_HPP
#define UI_MENU_HPP
#include "screen.hpp"
#include "button.hpp"
#include "mousemanager.hpp"
#include "scene/group.hpp"
#include "graphics/texture.hpp"
namespace UI {

class Menu : public Screen, private MouseManager {
public:
    Menu();
    virtual ~Menu();

    virtual void handleEvent(Event const &evt);
    virtual void draw(unsigned int ticks);

private:
    virtual Widget *traceMouse(Point pt);

    void newGame();
    void multiplayer();
    void options();
    void quit();

    bool initted_;
    Scene::Group scene_;
    Button menu_[4];
    Texture texture_;
};

}
#endif
