#include "menu.hpp"
#include "button.hpp"
#include "SDL_opengl.h"
#include <stdio.h>

UI::Menu::Menu()
    : initted_(false)
{ }

UI::Menu::~Menu()
{ }

void UI::Menu::handleEvent(Event const &evt)
{
    ui_.handleEvent(evt);
}

void UI::Menu::draw(unsigned int ticks)
{
    if (!initted_) {
        static char const *const MENU_ITEMS[4] = {
            "Single Player",
            "Multiplayer",
            "Options",
            "Quit"
        };
        for (int i = 0; i < 4; ++i) {
            menu_[i].setText(MENU_ITEMS[i]);
            menu_[i].setLoc(145, 345 - 50 * i);
            ui_.addChild(&menu_[i]);
        }
        menu_[0].setAction(Action(this, static_cast<Action::Method>
                                  (&Menu::newGame)));
        menu_[1].setAction(Action(this, static_cast<Action::Method>
                                  (&Menu::multiplayer)));
        menu_[2].setAction(Action(this, static_cast<Action::Method>
                                  (&Menu::options)));
        menu_[3].setAction(Action(this, static_cast<Action::Method>
                                  (&Menu::quit)));
        initted_ = true;
    }

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, 640.0, 0.0, 480.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);

    ui_.draw(ticks);
}

void UI::Menu::newGame()
{
    fputs("Menu: New Game\n", stdout);
}

void UI::Menu::multiplayer()
{
    fputs("Menu: Multiplayer\n", stdout);
}

void UI::Menu::options()
{
    fputs("Menu: Options\n", stdout);
}

void UI::Menu::quit()
{
    fputs("Menu: Quit\n", stdout);
}
