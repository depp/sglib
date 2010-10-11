#include "widget.hpp"
#include "event.hpp"

UI::Widget::Widget()
    : mouseWithinBounds_(false)
{
    bounds_.x = 0;
    bounds_.y = 0;
    bounds_.width = 0;
    bounds_.height = 0;
}

UI::Widget::~Widget()
{ }

void UI::Widget::handleEvent(UI::Event const &evt)
{
    switch (evt.type) {
    case MouseDown:
        mouseDown(evt.mouseEvent());
        break;
    case MouseUp:
        mouseUp(evt.mouseEvent());
        break;
    case MouseMove:
        mouseMoved(evt.mouseEvent());
        break;
    default:
        break;
    }
}

void UI::Widget::mouseMoved(UI::MouseEvent const &evt)
{
    if (mouseWithinBounds_) {
        if (!bounds().contains(evt.x, evt.y)) {
            mouseExited(evt);
            mouseWithinBounds_ = false;
        }
    } else {
        if (bounds().contains(evt.x, evt.y)) {
            mouseEntered(evt);
            mouseWithinBounds_ = true;
        }
    }
}

void UI::Widget::mouseEntered(UI::MouseEvent const &evt)
{ }

void UI::Widget::mouseExited(UI::MouseEvent const &evt)
{ }

void UI::Widget::mouseDown(UI::MouseEvent const &evt)
{ }

void UI::Widget::mouseUp(UI::MouseEvent const &evt)
{ }
