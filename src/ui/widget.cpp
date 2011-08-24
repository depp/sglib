#include "widget.hpp"
#include "event.hpp"

UI::Widget::Widget()
{
    bounds_.x = 0;
    bounds_.y = 0;
    bounds_.width = 0;
    bounds_.height = 0;
}

UI::Widget::~Widget()
{ }

bool UI::Widget::hitTest(Point pt)
{
    return bounds_.contains(pt.x, pt.y);
}

void UI::Widget::mouseMoved(MouseEvent const &)
{ }

void UI::Widget::mouseEntered(MouseEvent const &)
{ }

void UI::Widget::mouseExited(MouseEvent const &)
{ }

void UI::Widget::mouseDown(MouseEvent const &)
{ }

void UI::Widget::mouseUp(MouseEvent const &)
{ }
