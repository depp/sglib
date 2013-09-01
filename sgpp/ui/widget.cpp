/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "sgpp/ui/event.hpp"
#include "sgpp/ui/widget.hpp"

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
