#include "button.hpp"
#include "event.hpp"

UI::Button::Button()
    : title_(), state_(false), hover_(false), button_(-1)
{
    bounds_.width = 150;
    bounds_.height = 30;
}

UI::Button::~Button()
{ }

void UI::Button::setLoc(int x, int y)
{
    bounds_.x = x;
    bounds_.y = y;
}

void UI::Button::setText(std::string const &text)
{
    title_.setText(text);
}

void UI::Button::draw()
{
    glPushAttrib(GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT);

    float x1 = bounds_.x, x2 = bounds_.x + bounds_.width;
    float y1 = bounds_.y, y2 = bounds_.y + bounds_.height;

    if (!state_)
        glColor3ub(32, 0, 0);
    else
        glColor3ub(255, 128, 0);
    glBegin(GL_TRIANGLE_STRIP);
    glVertex2f(x1, y1);
    glVertex2f(x1, y2);
    glVertex2f(x2, y1);
    glVertex2f(x2, y2);
    glEnd();

    if (!state_)
        glColor3ub(255, 64, 64);
    else
        glColor3ub(64, 0, 0);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x1, y1);
    glVertex2f(x1, y2);
    glVertex2f(x2, y2);
    glVertex2f(x2, y1);
    glEnd();

    if (hover_ || button_ == ButtonLeft) {
        glColor3ub(255, 255, 255);
        glBegin(GL_LINE_LOOP);
        glVertex2f(x1 - 2.0f, y1 - 2.0f);
        glVertex2f(x1 - 2.0f, y2 + 2.0f);
        glVertex2f(x2 + 2.0f, y2 + 2.0f);
        glVertex2f(x2 + 2.0f, y1 - 2.0f);
        glEnd();
    }

    if (!state_)
        glColor3ub(255, 64, 64);
    else
        glColor3ub(64, 0, 0);
    glPushMatrix();
    glTranslatef(x1 + 5.0f, y1 + 10.0f, 0.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    title_.draw();
    glPopMatrix();

    glPopAttrib();
}

void UI::Button::mouseEntered(UI::MouseEvent const &evt)
{
    if (button_ == ButtonLeft)
        state_ = true;
    hover_ = true;
}

void UI::Button::mouseExited(UI::MouseEvent const &evt)
{
    if (button_ == ButtonLeft)
        state_ = false;
    hover_ = false;
}

void UI::Button::mouseDown(UI::MouseEvent const &evt)
{
    if (button_ < 0) {
        button_ = evt.button;
        if (button_ == ButtonLeft)
            state_ = bounds().contains(evt.x, evt.y);
    }
}

void UI::Button::mouseUp(UI::MouseEvent const &evt)
{
    bool doAction;
    if (button_ == evt.button) {
        doAction = state_ && button_ == ButtonLeft;
        button_ = -1;
        state_ = false;
        if (doAction)
            action_();
    }
}

void UI::Button::setAction(UI::Action const &action)
{
    action_ = action;
}
