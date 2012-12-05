/* Copyright 2012 Dietrich Epp <depp@zdome.net> */
#ifndef SGPP_UI_MOUSEMANAGER_HPP
#define SGPP_UI_MOUSEMANAGER_HPP
namespace UI {
struct MouseEvent;
class Widget;
struct Point;

/* A MouseManager dispatches mouse events to the correct widget.  This
   requires keeping a little bit of state so every mouse up event is
   delivered to the same widget which received the corresponding mouse
   down event.  This also dispatches mouse movement events as mouse
   enter and exit events.  */
class MouseManager {
public:
    MouseManager()
        : mouseWidget_(0), mouseButton_(-1), mouseWithinTarget_(false)
    { }

    void handleMouseEvent(MouseEvent const &evt);
    virtual Widget *traceMouse(Point pt) = 0;

private:
    Widget *mouseWidget_;
    int mouseButton_;
    bool mouseWithinTarget_;
};

}
#endif
