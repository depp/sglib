/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#ifndef SGPP_UI_WIDGET_HPP
#define SGPP_UI_WIDGET_HPP
#include "sgpp/scene/leafobject.hpp"
#include "sgpp/ui/geometry.hpp"
namespace UI {
struct MouseEvent;
struct KeyEvent;
struct Event;
class Screen;

class Widget : public Scene::LeafObject {
public:
    Widget();
    virtual ~Widget();

    virtual bool hitTest(Point pt);

    virtual void mouseMoved(MouseEvent const &evt);
    virtual void mouseEntered(MouseEvent const &evt);
    virtual void mouseExited(MouseEvent const &evt);
    virtual void mouseDown(MouseEvent const &evt);
    virtual void mouseUp(MouseEvent const &evt);

    Rect const &bounds() const { return bounds_; }

protected:
    Rect bounds_;
};

}
#endif
