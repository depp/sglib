#include "menu.hpp"
#include "type/type.hpp"
#include "SDL_opengl.h"

Menu::Menu()
    : UILayer(), title_(NULL)
{ }

Menu::~Menu()
{
    if (title_) delete title_;
}

void Menu::handleEvent(SDL_Event const &evt)
{
    
}

void Menu::draw()
{
    if (!title_) {
        title_ = new Type();
        title_->setText("Main Menu");
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
    glColor3ub(64, 64, 255);
    glPushMatrix();
    glTranslatef(200.0f, 360.0f, 0.0f);
    title_->draw();
    glPopMatrix();
    glPopAttrib();
}
