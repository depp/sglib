#include "menu.hpp"
#include "button.hpp"
#include "game/ld22/screen.hpp"
#include "game/tank/gamescreen.hpp"
#include "game/space/gamescreen.hpp"
#include "event.hpp"
#include "window.hpp"
#include "client/opengl.hpp"
#include <stdio.h>

UI::Menu::Menu()
    : initted_(false)
{ }

UI::Menu::~Menu()
{ }

void UI::Menu::handleEvent(Event const &evt)
{
    switch (evt.type) {
    case MouseDown:
    case MouseUp:
    case MouseMove:
        handleMouseEvent(evt.mouseEvent());
        break;
    default:
        break;
    }
}

void UI::Menu::update(unsigned int)
{
    if (!initted_) {
        initted_ = true;
        static const int MSIZE = 4;
        static char const *const MENU_ITEMS[4] = {
            "Tank game",
            "Space game",
            "LD22",
            "Quit"
        };
        for (int i = 0; i < MSIZE; ++i) {
            menu_[i].setText(MENU_ITEMS[i]);
            menu_[i].setLoc(145, 345 - 50 * i);
            scene_.addObject(&menu_[i]);
        }
        texture_ = Texture::file("font/cp437-8x8.png");
        texture2_ = Texture::file("font/cp437-8x8.png");
        menu_[0].setAction(Action(this, static_cast<Action::Method>
                                  (&Menu::tankGame)));
        menu_[1].setAction(Action(this, static_cast<Action::Method>
                                  (&Menu::spaceGame)));
        menu_[2].setAction(Action(this, static_cast<Action::Method>
                                  (&Menu::ld22)));
        menu_[3].setAction(Action(this, static_cast<Action::Method>
                                  (&Menu::quit)));
    }
}

void UI::Menu::draw()
{
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, window().width(), 0.0, window().height(), -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    scene_.draw();

    glPushAttrib(GL_ENABLE_BIT);
    glEnable(GL_TEXTURE_2D);
    texture_->bind();
    glBegin(GL_TRIANGLE_STRIP);
    glTexCoord2f(0.0f, 1.0f); glVertex2f(100.0f, 100.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex2f(228.0f, 100.0f);
    glTexCoord2f(0.0f, 0.0f); glVertex2f(100.0f, 228.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex2f(228.0f, 228.0f);
    glEnd();
    glPopAttrib();
}

UI::Widget *UI::Menu::traceMouse(UI::Point pt)
{
    std::vector<Scene::LeafObject *> t;
    scene_.trace(t, pt);
    std::vector<Scene::LeafObject *>::iterator
        i = t.begin(), e = t.end();
    for (; i != e; ++i) {
        Widget *w = dynamic_cast<Widget *>(*i);
        if (w)
            return w;
    }
    return 0;
}

void UI::Menu::tankGame()
{
    window().setScreen(new Tank::GameScreen);
}

void UI::Menu::spaceGame()
{
    window().setScreen(new Space::GameScreen);
}

void UI::Menu::ld22()
{
    window().setScreen(new LD22::Screen);
}

void UI::Menu::quit()
{
    window().close();
}
