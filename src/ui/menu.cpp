#include "menu.hpp"
#include "button.hpp"
#include "SDL_opengl.h"

UI::Menu::Menu()
    : initted_(false)
{
    for (int i = 0; i < 4; ++i)
        menu_[i] = NULL;
}

UI::Menu::~Menu()
{ }

void UI::Menu::handleEvent(SDL_Event const &evt)
{ }

void UI::Menu::draw()
{
    if (!initted_) {
        char const *const FAMILY[] = {
            "Helvetica",
            "Arial",
            "DejaVu Sans",
            "Bitstream Vera Sans",
            NULL
        };
        Font font;
        font.setFamily(FAMILY);
        font.setSize(36.0f);
        font.setWeight(Font::WeightBold);
        title_.setText("Cyber Zone");
        title_.setFont(font);
        title_.setAlignment(RasterText::Center);

        for (int i = 0; i < 4; ++i) {
            menu_[i] = new Button;
            menu_[i]->setLoc(150.0f, 350.0f - 50.0f * i);
        }
        menu_[0]->setText("Single Player");
        menu_[1]->setText("Multiplayer");
        menu_[2]->setText("Options");
        menu_[3]->setText("Quit");
        initted_ = true;
    }

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, 640.0, 0.0, 480.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);

    glLoadIdentity();
    glPushAttrib(GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor3ub(255, 0, 0);
    glPushMatrix();
    glTranslatef(320.0f, 400.0f, 0.0f);
    title_.draw();
    glPopMatrix();
    glPopAttrib();

    for (int i = 0; i < 4; ++i)
        menu_[i]->draw();
}
