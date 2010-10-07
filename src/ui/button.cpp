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
    glPushAttrib(GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT);
    glPushMatrix();
    glTranslatef(x_, y_, 0.0f);

    glColor3ub(255, 0, 0);

    float x1 = -5.0f, x2 = 150.0f, y1 = -10.0f, y2 = 20.0f;
    glBegin(GL_LINE_LOOP);
    glVertex2f(x1, y1);
    glVertex2f(x1, y2);
    glVertex2f(x2, y2);
    glVertex2f(x2, y1);
    glEnd();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    title_.draw();

    glPopAttrib();
    glPopMatrix();
}
