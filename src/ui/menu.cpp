#include "menu.hpp"
#include "button.hpp"
#include "game/tank/gamescreen.hpp"
#include "event.hpp"
#include "graphics/video.hpp"
#include "graphics/texturefile.hpp"
#include "opengl.hpp"
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
        static char const *const MENU_ITEMS[4] = {
            "Single Player",
            "Multiplayer",
            "Options",
            "Quit"
        };
        for (int i = 0; i < 4; ++i) {
            menu_[i].setText(MENU_ITEMS[i]);
            menu_[i].setLoc(145, 345 - 50 * i);
            scene_.addObject(&menu_[i]);
        }
        texture_ = TextureFile::open("font/cp437-8x8.png");
        texture2_ = TextureFile::open("font/cp437-8x8.png");
        menu_[0].setAction(Action(this, static_cast<Action::Method>
                                  (&Menu::newGame)));
        menu_[1].setAction(Action(this, static_cast<Action::Method>
                                  (&Menu::multiplayer)));
        menu_[2].setAction(Action(this, static_cast<Action::Method>
                                  (&Menu::options)));
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
    glOrtho(0.0, Video::width, 0.0, Video::height, -1.0, 1.0);
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

void UI::Menu::newGame()
{
    setActive(new Tank::GameScreen);
}

void UI::Menu::multiplayer()
{ }

void UI::Menu::options()
{ }

void UI::Menu::quit()
{
    setActive(NULL);
}
