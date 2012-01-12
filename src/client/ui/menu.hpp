#ifndef CLIENT_UI_MENU_HPP
#define CLIENT_UI_MENU_HPP
#include "screen.hpp"
#include "button.hpp"
#include "mousemanager.hpp"
#include "client/scene/group.hpp"
#include "client/texture.hpp"
namespace UI {

class Menu : public Screen, private MouseManager {
public:
    Menu();
    virtual ~Menu();

    virtual void handleEvent(Event const &evt);
    virtual void update(unsigned int ticks);
    virtual void draw(Viewport &v, unsigned msec);

private:
    virtual Widget *traceMouse(Point pt);

    void tankGame();
    void spaceGame();
    void ld22();
    void quit();

    bool initted_;
    Scene::Group scene_;
    Button menu_[4];
    Texture::Ref texture_, texture2_;
};

}
#endif
