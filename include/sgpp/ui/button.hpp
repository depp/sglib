/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#ifndef SGPP_UI_BUTTON_HPP
#define SGPP_UI_BUTTON_HPP
#include "sgpp/type.hpp"
#include "sgpp/ui/action.hpp"
#include "sgpp/ui/widget.hpp"
#include <string>
namespace UI {

class Button : public Widget {
public:
    Button();
    virtual ~Button();

    void setLoc(int x, int y);
    void setText(std::string const &text);

    virtual void draw();

    // virtual void mouseMoved(MouseEvent const &evt);
    virtual void mouseEntered(MouseEvent const &evt);
    virtual void mouseExited(MouseEvent const &evt);
    virtual void mouseDown(MouseEvent const &evt);
    virtual void mouseUp(MouseEvent const &evt);

    void setAction(Action const &action);

private:
    // RasterText::Ref title_;
    TextLayout title_;
    bool state_, hover_;
    int button_;
    Action action_;
};

}
#endif
