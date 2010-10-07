#include "button.hpp"

UI::Button::Button()
    : title_(), x_(0.0f), y_(0.0f)
{ }

UI::Button::~Button()
{ }

void UI::Button::setLoc(float x, float y)
{
    x_ = x;
    y_ = y;
}

void UI::Button::setText(std::string const &text)
{
    title_.setText(text);
}

void UI::Button::draw()
{
    glPushMatrix();
    glTranslatef(x_, y_, 0.0f);
    glPushAttrib(GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor3ub(255, 0, 0);
    title_.draw();
    glPopAttrib();
    glPopMatrix();
}
