/* Copyright 2012 Dietrich Epp <depp@zdome.net> */
#include "sgpp/ui/mousemanager.hpp"
#include "sgpp/ui/event.hpp"
#include "sgpp/ui/widget.hpp"

void UI::MouseManager::handleMouseEvent(MouseEvent const &evt)
{
    Widget *w, *wo;
    bool f, fo;
    if (mouseButton_ >= 0) {
        switch (evt.type) {
        case MouseDown:
            if (mouseWidget_)
                mouseWidget_->mouseDown(evt);
            break;
        case MouseUp:
            if (mouseButton_ == evt.button)
                mouseButton_ = -1;
            if (mouseWidget_)
                mouseWidget_->mouseUp(evt);
            break;
        case MouseMove:
            if (mouseWidget_) {
                f = mouseWidget_->hitTest(Point(evt.x, evt.y));
                fo = mouseWithinTarget_;
                if (f != fo) {
                    mouseWithinTarget_ = f;
                    if (f)
                        mouseWidget_->mouseEntered(evt);
                    else
                        mouseWidget_->mouseExited(evt);
                } else if (f)
                    mouseWidget_->mouseMoved(evt);
            }
            break;
        default:
            break;
        }
    } else {
        switch (evt.type) {
        case MouseDown:
            w = traceMouse(Point(evt.x, evt.y));
            mouseWidget_ = w;
            mouseButton_ = evt.button;
            mouseWithinTarget_ = true;
            if (w)
                w->mouseDown(evt);
            break;
        case MouseUp:
            break;
        case MouseMove:
            wo = mouseWidget_;
            w = traceMouse(Point(evt.x, evt.y));
            if (w == wo) {
                if (w)
                    w->mouseMoved(evt);
            } else {
                mouseWidget_ = w;
                if (wo)
                    wo->mouseExited(evt);
                if (w)
                    w->mouseEntered(evt);
            }
            break;
        default:
            break;
        }
    }
}
