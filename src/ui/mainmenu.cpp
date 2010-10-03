#include "mainmenu.hpp"

MainMenu::MainMenu()
    : initted_(false)
{ }

MainMenu::~MainMenu()
{ }

void MainMenu::render()
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
        Font titlefont(font), itemfont(font);
        titlefont.setSize(36.0f);
        titlefont.setWeight(Font::WeightBold);
        itemfont.setSize(18.0f);
        itemfont.setStyle(Font::Italic);
        title_.setText("Cyber Zone");
        title_.setFont(titlefont);
        title_.setAlignment(RasterText::Center);
        for (int i = 0; i < 4; ++i)
            menu_[i].setFont(itemfont);
        menu_[0].setText("Single Player");
        menu_[1].setText("Multiplayer");
        menu_[2].setText("Options");
        menu_[3].setText("Quit");
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
    glColor3ub(255, 255, 0);
    for (int i = 0; i < 4; ++i) {
        glPushMatrix();
        glTranslatef(150.0f, 350.0f - 50.0f * i, 0.0f);
        menu_[i].draw();
        glPopMatrix();
    }
    glPopAttrib();
}
